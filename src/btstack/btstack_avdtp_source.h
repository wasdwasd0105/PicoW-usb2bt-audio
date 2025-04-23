//
// Created by Sean on 8/7/23.
//

#ifndef PICOW_USB_BT_AUDIO_SSP_COUNTER_H
#define PICOW_USB_BT_AUDIO_SSP_COUNTER_H

#define AUDIO_BUF_POOL_LEN 5120

int get_bt_buf_counter();

void set_usb_buf_counter(uint16_t counter);

void set_shared_audio_buffer(int16_t *data);

int btstack_main(int argc, const char * argv[]);

void avdtp_disconnect_and_scan(void);

void gap_start_scanning(void);

bool get_a2dp_connected_flag();

void a2dp_source_reconnect();

void avdtp_source_establish_stream();

void set_next_capablity_and_start_stream();

void start_led_blink();

static int setup_aac_configuration();

static int setup_sbc_configuration();

static int setup_aptx_configuration();

static int setup_aptx_hd_configuration();
static int set_ldac_configuration();



#endif //PICOW_USB_BT_AUDIO_SSP_COUNTER_H
