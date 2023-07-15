// AUDIO_BUF_POOL_LEN is the len of pool that share usb audio to bt
#define AUDIO_BUF_POOL_LEN 4096

// enable to show bt debug info
//#define HAVE_BTSTACK_STDIN

int btstack_main(int argc, const char * argv[]);

bool bt_audio_steam_ready(void);

void set_shared_audio_buffer(int16_t *data);
bool get_audio_buffer_lock();

void bt_disconnect_and_scan();

void bt_usb_resync_counter();

bool get_a2dp_connected_flag();

void a2dp_source_reconnect();

static void get_first_link_key();

static bool bt_audio_ready;