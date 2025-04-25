#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"
#include "btstack_run_loop.h"
#include "btstack/bt_audio.h"
#include <stdio.h>
#include "pico/multicore.h"

#include "btstack_event.h"

#include "hardware/gpio.h"
#include "hardware/sync.h"
#include "hardware/structs/ioqspi.h"
#include "hardware/structs/sio.h"
#include "hardware/clocks.h"
#include "hardware/vreg.h"
#include "hardware/timer.h"

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


// by wasdwasd0105

int bootsel_state_counter = 0;

void check_bootsel_state(){
    bool current_state = get_bootsel_button();

    if(current_state){
        bootsel_state_counter++;
    }
    else{

        if(bootsel_state_counter > 50){
            printf("key prassed long!\n");
            bt_disconnect_and_scan();
        }

        else if (bootsel_state_counter > 5){
            printf("key prassed short!\n");
            if (get_a2dp_connected_flag() == false){
                a2dp_source_reconnect();
            }else{
            }

        }
        bootsel_state_counter = 0;
    }

}

static btstack_packet_callback_registration_t hci_event_callback_registration;


static void packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size){
    UNUSED(size);
    UNUSED(channel);
    bd_addr_t local_addr;
    if (packet_type != HCI_EVENT_PACKET) return;
    switch(hci_event_packet_get_type(packet)){
        case BTSTACK_EVENT_STATE:
            if (btstack_event_state_get_state(packet) != HCI_STATE_WORKING) return;
            gap_local_bd_addr(local_addr);
            printf("BTstack up and running on %s.\n", bd_addr_to_str(local_addr));
            break;
        default:
            break;
    }
}


bool usb_timer_callback(repeating_timer_t *rt){
    tinyusb_task();
    return true;
}


int main() {

    // vreg_set_voltage(VREG_VOLTAGE_1_20);
    // sleep_ms(100);
    // set_sys_clock_khz(250000, true);

    // // enable to use uart see debug info
    stdio_init_all();
    stdout_uart_init();

    flash_safe_execute_core_init();

    tinyusb_main();

    printf("init ctw43.\n");

    // initialize CYW43 driver
    if (cyw43_arch_init()) {
        printf("cyw43_arch_init() failed.\n");
        return -1;
    }

    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);


    // inform about BTstack state
    hci_event_callback_registration.callback = &packet_handler;
    hci_add_event_handler(&hci_event_callback_registration);

    btstack_main(0, NULL);
    sleep_ms(200);  
    //bt_disconnect_and_scan();
    
    static repeating_timer_t usb_timer;
    add_repeating_timer_us(-25, usb_timer_callback, NULL, &usb_timer);

    while (1) {
        //printf("get_bootsel_button is %d\n", get_bootsel_button());
        check_bootsel_state();
        tinyusb_control_task();
        sleep_ms(20);
    }

}
