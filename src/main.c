#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"
#include "btstack_run_loop.h"

#include "btstack/btstack_avdtp_source.h"
#include "btstack/btstack_hci.h"

#include <stdio.h>
#include "pico/stdlib.h"

#include "pico/multicore.h"

#include "btstack_event.h"

#include "hardware/flash.h"
#include "hardware/gpio.h"
#include "hardware/sync.h"
#include "hardware/structs/ioqspi.h"
#include "hardware/structs/sio.h"
#include "hardware/clocks.h"
#include "hardware/vreg.h"
#include "hardware/timer.h"
#include "hardware/structs/ioqspi.h"
#include "hardware/structs/sio.h"
#include "hardware/watchdog.h"

#include "tinyusb/uac.h"
#include "pico_w_led.h"
#include "pico/flash.h"


bool __no_inline_not_in_flash_func(get_bootsel_button)() {
    const uint CS_PIN_INDEX = 1;

    // Must disable interrupts, as interrupt handlers may be in flash, and we
    // are about to temporarily disable flash access!
    uint32_t flags = save_and_disable_interrupts();

    // Set chip select to Hi-Z
    hw_write_masked(&ioqspi_hw->io[CS_PIN_INDEX].ctrl,
                    GPIO_OVERRIDE_LOW << IO_QSPI_GPIO_QSPI_SS_CTRL_OEOVER_LSB,
                    IO_QSPI_GPIO_QSPI_SS_CTRL_OEOVER_BITS);

    // Note we can't call into any sleep functions in flash right now
    for (volatile int i = 0; i < 1000; ++i);

    // The HI GPIO registers in SIO can observe and control the 6 QSPI pins.
    // Note the button pulls the pin *low* when pressed.
#if PICO_RP2040
    #define CS_BIT (1u << 1)
#else
    #define CS_BIT SIO_GPIO_HI_IN_QSPI_CSN_BITS
#endif
    bool button_state = !(sio_hw->gpio_hi_in & CS_BIT);

    // Need to restore the state of chip select, else we are going to have a
    // bad time when we return to code in flash!
    hw_write_masked(&ioqspi_hw->io[CS_PIN_INDEX].ctrl,
                    GPIO_OVERRIDE_NORMAL << IO_QSPI_GPIO_QSPI_SS_CTRL_OEOVER_LSB,
                    IO_QSPI_GPIO_QSPI_SS_CTRL_OEOVER_BITS);

    restore_interrupts(flags);

    return button_state;
}

void on_single_press(void){
    printf("key pressed short (single tap)!\n");
    if (! get_a2dp_connected_flag()) {
        a2dp_source_reconnect();
    } else {
        // next short-press action
    }
}

void on_double_press(void){
    if(!get_allow_switch_slot()){
        return;
    }
    uint8_t currect_slot = read_uint8_last_flash();
    if (currect_slot == 0x1){
        printf("switch to slot 2!\n");
        set_led_mode_off();
        set_led_mode_triple_blink();
        write_uint8_last_flash(0x2);
    } else{
        printf("switch to slot 1!\n");
        set_led_mode_off();
        set_led_mode_double_blink();
        write_uint8_last_flash(0x1);
    }
    get_link_keys();
    printf("key pressed double!\n");
}

void on_long_press(void){
    printf("key pressed long!\n");
    avdtp_disconnect_and_scan();
}

// ---------- parameters ----------
#define DEBOUNCE_US          5000          // 5 ms
#define SHORT_PRESS_MIN_US  100000         // 100 ms
#define LONG_PRESS_MIN_US  1000000         // 1  s
#define DOUBLE_WINDOW_US    500000         // 500 ms

// ---------- state ----------
static bool     debounced_state  = false;   // last stable level
static uint64_t last_edge_us     = 0;       // last time level changed
static uint64_t press_start_us   = 0;       // when current press began
static uint64_t last_release_us  = 0;       // when previous press ended
static uint8_t  tap_counter      = 0;

// call every 1 ms
void check_bootsel_state(void)
{
    bool raw = get_bootsel_button();            // true == pressed
    uint64_t now = time_us_64();

    // --- debounce ---
    if (raw != debounced_state) {
        if (now - last_edge_us >= DEBOUNCE_US) {
            debounced_state = raw;
            last_edge_us = now;

            if (debounced_state) {              // -------- press --------
                press_start_us = now;
            } else {                            // -------- release ------
                uint64_t press_len = now - press_start_us;

                // classify type of this press
                if (press_len >= LONG_PRESS_MIN_US) {
                    on_long_press();
                    tap_counter = 0;            // long press stands alone
                } else if (press_len >= SHORT_PRESS_MIN_US) {
                    tap_counter++;
                    if (tap_counter == 1) {
                        last_release_us = now;  // start double-tap window
                    } else if (tap_counter == 2 &&
                               (now - last_release_us) <= DOUBLE_WINDOW_US) {
                        on_double_press();
                        tap_counter = 0;
                    }
                }
            }
        }
    }

    // timeout: single tap
    if (tap_counter == 1 && (now - last_release_us) > DOUBLE_WINDOW_US) {
        on_single_press();
        tap_counter = 0;
    }
}






bool usb_timer_callback(repeating_timer_t *rt){
    tinyusb_task();
    return true;
}

bool bootsel_timer_callback(repeating_timer_t *rt) {
    (void)rt;
    check_bootsel_state();
    return true;    // keep repeating
}

int main() {

    vreg_set_voltage(VREG_VOLTAGE_1_20);
    sleep_ms(100);
    set_sys_clock_khz(250000, true);

    // // enable to use uart see debug info
    stdio_init_all();
    stdout_uart_init();

    flash_safe_execute_core_init();

    uint8_t currect_slot = read_uint8_last_flash();

    printf("init slot is %d\n", currect_slot);

    if (currect_slot != 0x1 && currect_slot != 0x2){
        printf("Doesn't has slot, set the slot to 1\n");
        write_uint8_last_flash(0x1);
        currect_slot = read_uint8_last_flash();
    }

    tinyusb_main();

    printf("init ctw43.\n");

    // initialize CYW43 driver
    if (cyw43_arch_init()) {
        printf("cyw43_arch_init() failed.\n");
        return -1;
    }

    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);

    btstack_main(0, NULL);
    sleep_ms(200);  
    
    static repeating_timer_t usb_timer;
    add_repeating_timer_us(-30, usb_timer_callback, NULL, &usb_timer);

    static repeating_timer_t bootsel_timer;
    add_repeating_timer_us(20000, bootsel_timer_callback, NULL, &bootsel_timer);

    while (1) {
        tinyusb_control_task();
        sleep_ms(50);
    }

}
