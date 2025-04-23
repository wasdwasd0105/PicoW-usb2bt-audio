#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"


static uint16_t repeating_time = 500;
static uint16_t repeating_time_short = 200;
static bool is_repeating = false;
static bool led_static_state = false;
static bool led_on = true;
static bool led_short = true;

static void led_do_work(async_context_t *context, async_at_time_worker_t *worker);
async_at_time_worker_t led_timeout = { .do_work = led_do_work };

static void led_do_work(async_context_t *context, async_at_time_worker_t *worker) {
    // Invert the led

    led_on = !led_on;
    led_short = !led_short;

    if (!is_repeating){
        led_on = led_static_state;
    }

    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, led_on);
    if (is_repeating){
        async_context_add_at_time_worker_in_ms(context, worker, (led_short ? repeating_time : repeating_time_short) );
    }
}


void set_led_mode_pairing(){
    // In main start the process off - AFTER having called cyw43_arch_init...
    led_on = true;
    led_short = true;
    is_repeating = true;
    repeating_time = 100;
    repeating_time_short = 100;
    async_context_add_at_time_worker_in_ms(cyw43_arch_async_context(), &led_timeout, repeating_time);
}

void set_led_mode_playing_sbc(){
    led_on = true;
    led_short = true;
    is_repeating = true;
    repeating_time = 1000;
    repeating_time_short = 1000;
    async_context_add_at_time_worker_in_ms(cyw43_arch_async_context(), &led_timeout, repeating_time);
}

void set_led_mode_playing_ldac(){
    led_on = true;
    led_short = true;
    is_repeating = true;
    repeating_time_short = 1000;
    repeating_time = 200;
    async_context_add_at_time_worker_in_ms(cyw43_arch_async_context(), &led_timeout, repeating_time);
}



void set_led_mode_off(){
    led_on = true;
    led_short = true;
    is_repeating = false;
    led_static_state = false;
}


void set_led_mode_on(){
    led_on = true;
    led_short = true;
    is_repeating = false;
    led_static_state = true;
}