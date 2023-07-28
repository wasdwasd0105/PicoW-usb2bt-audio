#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"


uint16_t repeating_time = 500;
bool is_repeating = false;
bool led_static_state = false;

static void led_do_work(async_context_t *context, async_at_time_worker_t *worker);
async_at_time_worker_t led_timeout = { .do_work = led_do_work };

static void led_do_work(async_context_t *context, async_at_time_worker_t *worker) {
    // Invert the led
    static int led_on = true;
    led_on = !led_on;

    if (!is_repeating){
        led_on = led_static_state;
    }

    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, led_on);
    if (is_repeating){
        async_context_add_at_time_worker_in_ms(context, worker, repeating_time);
    }
}


void set_led_mode_pairing(){
    // In main start the process off - AFTER having called cyw43_arch_init...
    is_repeating = true;
    repeating_time = 100; 
    async_context_add_at_time_worker_in_ms(cyw43_arch_async_context(), &led_timeout, repeating_time);
}

void set_led_mode_playing(){
    is_repeating = true;
    repeating_time = 1000; 
    async_context_add_at_time_worker_in_ms(cyw43_arch_async_context(), &led_timeout, repeating_time);
}


void set_led_mode_off(){
    is_repeating = false;
    led_static_state = false;
}


void set_led_mode_on(){
    is_repeating = false;
    led_static_state = true;
}