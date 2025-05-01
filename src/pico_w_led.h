#define MAC_LEN               6

void set_led_mode_pairing();

void set_led_mode_playing_sbc();

void set_led_mode_playing_ldac();

void set_led_mode_off();

void set_led_mode_triple_blink();

void set_led_mode_double_blink();

void stop_triple_blink();

void stop_double_blink();

uint8_t read_uint8_last_flash();

bool write_uint8_last_flash(uint8_t value);

void read_slot1_mac(uint8_t mac[MAC_LEN]);

bool write_slot1_mac(const uint8_t mac[MAC_LEN]);

void read_slot2_mac(uint8_t mac[MAC_LEN]);

bool write_slot2_mac(const uint8_t mac[MAC_LEN]);