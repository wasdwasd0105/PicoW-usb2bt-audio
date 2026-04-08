#include <sys/cdefs.h>
/*
 * Copyright (C) 2016 BlueKitchen GmbH
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holders nor the names of
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 * 4. Any redistribution, use, or modification is done solely for
 *    personal benefit and not for any commercial purpose or for
 *    monetary gain.
 *
 * THIS SOFTWARE IS PROVIDED BY BLUEKITCHEN GMBH AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL MATTHIAS
 * RINGWALD OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * Please inquire about commercial licensing options at 
 * contact@bluekitchen-gmbh.com
 *
 */


/*
 * 
*/

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "btstack.h"
#include "btstack_avdtp_source.h"
#include "pico/multicore.h"
#include "pico/util/queue.h"

#include "btstack_hci.h"

#include "../pico_w_led.h"

#define HAVE_AAC_FDK

#define HAVE_LDAC_ENCODER
#include <ldacBT.h>

#define A2DP_CODEC_VENDOR_ID_SONY 0x12d
#define A2DP_SONY_CODEC_LDAC 0xaa

#define A2DP_CODEC_VENDOR_ID_APPLE 0x004C
#define A2DP_APPLE_CODEC_AAC_ELD 0x8001


#include <aacenc_lib.h>


#define A2DP_CODEC_VENDOR_ID_FRAUNHOFER 0x08A9
#define A2DP_FRAUNHOFER_CODEC_LC3PLUS 0x0001

#define AVDTP_MAX_SEP_NUM 10
#define AVDTP_MAX_MEDIA_CODEC_CONFIG_LEN 16
#define AVDTP_MAX_MEDIA_CODEC_CAPABILITES_EVENT_LEN 100

#define VOLUME_REDUCTION 2

typedef struct {
    // bitmaps
    uint8_t sampling_frequency_bitmap;
    uint8_t channel_mode_bitmap;
    uint8_t block_length_bitmap;
    uint8_t subbands_bitmap;
    uint8_t allocation_method_bitmap;
    uint8_t min_bitpool_value;
    uint8_t max_bitpool_value;
} media_codec_information_sbc_t;

typedef struct {
    int reconfigure;
    int num_channels;
    int sampling_frequency;
    int block_length;
    int subbands;
    int min_bitpool_value;
    int max_bitpool_value;

    int channel_mode;
    int allocation_method;
} avdtp_media_codec_configuration_sbc_t;


typedef struct {
    int reconfigure;
    int channel_mode;
    int num_channels;
    int num_samples;
    int sampling_frequency;
} avdtp_media_codec_configuration_ldac_t;


#ifdef HAVE_BTSTACK_STDIN

#endif

typedef struct {
    uint16_t avdtp_cid;
    uint8_t  local_seid;
    uint8_t  remote_seid;
    uint16_t avrcp_cid;

    uint32_t time_audio_data_sent; // ms
    uint32_t acc_num_missed_samples;
    uint32_t samples_ready;
    btstack_timer_source_t audio_timer;
    uint8_t  streaming;
    int      max_media_payload_size;

    uint32_t rtp_timestamp;


    uint8_t  codec_storage[1500];  // 3 AAC-ELD frames: 3*~400 = 1200 + margin
    uint16_t codec_storage_count;
    uint8_t  codec_ready_to_send;
    uint16_t codec_num_frames;
    uint8_t volume;
}  a2dp_media_sending_context_t;


#ifdef HAVE_LDAC_ENCODER
HANDLE_LDAC_BT handleLDAC;
#endif


#ifdef HAVE_AAC_FDK
static HANDLE_AACENCODER handleAAC;
static AACENC_InfoStruct aacinf;
#endif

static a2dp_media_sending_context_t media_tracker;

static int current_sample_rate = 48000;

static uint8_t sdp_avdtp_source_service_buffer[150];

static uint16_t     num_remote_seps;
static uint16_t     cur_num_remote_seps;

static uint8_t cur_capability = 0;
static bool is_streaming = false;
static uint8_t cur_codec = 0;

static void avrcp_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size);
static void avrcp_target_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size);
static void avrcp_controller_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size);

typedef struct {
    uint8_t track_id[8];
    uint32_t song_length_ms;
    avrcp_playback_status_t status;
    uint32_t song_position_ms; // 0xFFFFFFFF if not supported
} avrcp_play_status_info_t;


avrcp_track_t tracks[] = {
    {{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02}, 2, "USB Audio", "Pico", "wasdwasd0105", "vivid", 12345},
};

avrcp_play_status_info_t play_info;

static uint8_t sdp_avrcp_target_service_buffer[200];
static uint8_t sdp_avrcp_controller_service_buffer[200];


static uint16_t                  media_codec_config_len;
static uint8_t                   media_codec_config_data[AVDTP_MAX_MEDIA_CODEC_CONFIG_LEN];

static struct {
    avdtp_sep_t  sep;
    bool have_media_codec_apabilities;
    uint8_t                  media_codec_event[AVDTP_MAX_MEDIA_CODEC_CAPABILITES_EVENT_LEN];
    uint32_t                 vendor_id;
    uint32_t                 codec_id;
} remote_seps[AVDTP_MAX_SEP_NUM];
static uint8_t selected_remote_sep_index;

static struct {
    avdtp_stream_endpoint_t * local_stream_endpoint;
    uint32_t sampling_frequency;
}  sc;


// setup stream endpoints

avdtp_stream_endpoint_t * stream_endpoint_sbc;
avdtp_stream_endpoint_t * stream_endpoint_ldac;
avdtp_stream_endpoint_t * stream_endpoint_aac;
avdtp_stream_endpoint_t * stream_endpoint_aaceld;


static btstack_sbc_encoder_state_t sbc_encoder_state;
static uint8_t is_cmd_triggered_localy = 0;

static bool a2dp_is_connected_flag = false;

static bool finish_scan_avdtp_codec = false;

static uint8_t audio_timer_interval = 5;

// on pico 2w the max stable aac bit rate under 512 simples without vbr is around 220000
static uint8_t aac_audio_timer_interval = 12;
static uint16_t acc_num_simples = 1024;
static int max_aac_bit_rate = 220000;
static int aac_bit_rate = 192000;


static const uint8_t media_sbc_codec_capabilities[] = {
    0xFF,//(AVDTP_SBC_44100 << 4) | AVDTP_SBC_STEREO,
    0xFF,//(AVDTP_SBC_BLOCK_LENGTH_16 << 4) | (AVDTP_SBC_SUBBANDS_8 << 2) | AVDTP_SBC_ALLOCATION_METHOD_LOUDNESS,
    2, 53
};


static uint8_t media_ldac_codec_capabilities[] = {
        0x2D, 0x1, 0x0, 0x0,
        0xAA, 0,
        0x30,  // 0x20 (44100) | 0x10 (48000)
        0x01,
        0x1
};

#ifdef HAVE_AAC_FDK
static uint8_t media_aac_codec_capabilities[] = {
        0xF0,
        0xFF,
        0xFC,
        0x80,
        0,
        0
};

typedef struct {
    int reconfigure;
    int sampling_frequency_bitmap;
    int object_type_bitmap;
    int bit_rate;
    int channels_bitmap;
    int vbr;
} avdtp_media_codec_capabilities_aac_t;

#endif

// Apple AAC-ELD vendor codec capabilities (vendor_id=0x004C, codec_id=0x8001)
// Format: vendor_id(4 LE) + codec_id(2 LE) + obj_type(2) + freq+ch(2) + rsvd(1) + vbr+bitrate(3)
static uint8_t media_aaceld_codec_capabilities[] = {
        0x4C, 0x00, 0x00, 0x00,   // vendor_id = 0x004C (Apple)
        0x01, 0x80,                // codec_id  = 0x8001
        0x00, 0x80,                // object_type = 0x0080 (AAC-ELD)
        0x00, 0x8C,                // freq bitmap 0x008 (48kHz) + channels 0xC (mono+stereo)
        0x00,                      // reserved
        0x83, 0xE8, 0x00           // VBR=1, bitrate=256000
};

// configurations for local stream endpoints
static uint8_t local_stream_endpoint_sbc_media_codec_configuration[4];
static uint8_t local_stream_endpoint_ldac_media_codec_configuration[9];
static uint8_t local_stream_endpoint_aaceld_media_codec_configuration[14];
static avdtp_media_codec_configuration_ldac_t ldac_configuration;

#ifdef HAVE_AAC_FDK
static uint8_t local_stream_endpoint_aac_media_codec_configuration[6];
#endif

bool get_a2dp_connected_flag(){
    return a2dp_is_connected_flag;
}

static int a2dp_sample_rate(void){
    return current_sample_rate;
}

static void configure_sample_rate(int sampling_frequency){
    switch (sampling_frequency){
        case AVDTP_SBC_48000:
            current_sample_rate = 48000;
            break;
        case AVDTP_SBC_44100:
            current_sample_rate = 44100;
            break;
        case AVDTP_SBC_32000:
            current_sample_rate = 32000;
            break;
        case AVDTP_SBC_16000:
            current_sample_rate = 16000;
            break;
        default:
            break;
    }

    media_tracker.codec_storage_count = 0;
    media_tracker.samples_ready = 0;
}

static const char * codec_name_for_type(avdtp_media_codec_type_t codec_type){
    switch (codec_type){
        case AVDTP_CODEC_SBC:            return "SBC";
        case AVDTP_CODEC_MPEG_1_2_AUDIO: return "MPEG_1_2_AUDIO";
        case AVDTP_CODEC_MPEG_2_4_AAC:   return "MPEG_2_4_AAC";
        case AVDTP_CODEC_ATRAC_FAMILY:   return "ATRAC_FAMILY";
        case AVDTP_CODEC_NON_A2DP:       return "NON_A2DP/Vendor";
        default:                         return "Unknown";
    }
}

static void a2dp_demo_send_media_packet_sbc(void){
    int num_bytes_in_frame = btstack_sbc_encoder_sbc_buffer_length();
    int bytes_in_storage = media_tracker.codec_storage_count;
    uint8_t num_sbc_frames = bytes_in_storage / num_bytes_in_frame;
    // Prepend SBC Header
    media_tracker.codec_storage[0] = num_sbc_frames;  // (fragmentation << 7) | (starting_packet << 6) | (last_packet << 5) | num_frames;
    a2dp_source_stream_send_media_payload_rtp(media_tracker.avdtp_cid, media_tracker.local_seid, 0,
                                               media_tracker.rtp_timestamp,
                                               media_tracker.codec_storage, bytes_in_storage + 1);



    // update rtp_timestamp
    unsigned int num_audio_samples_per_sbc_buffer = btstack_sbc_encoder_num_audio_frames();

    media_tracker.rtp_timestamp += num_sbc_frames * num_audio_samples_per_sbc_buffer;

    media_tracker.codec_storage_count = 0;
    media_tracker.codec_ready_to_send = 0;
}


static void a2dp_demo_send_media_packet_aac(void) {
    uint8_t num_frames = 1;

    a2dp_source_stream_send_media_payload_rtp(media_tracker.avdtp_cid, media_tracker.local_seid, 0, media_tracker.rtp_timestamp, &media_tracker.codec_storage[0], media_tracker.codec_storage_count);

    //media_tracker.rtp_timestamp += aacinf.frameLength;
    media_tracker.rtp_timestamp += acc_num_simples;

    media_tracker.codec_storage_count = 0;
    media_tracker.codec_ready_to_send = 0;
    media_tracker.codec_num_frames = 0;

}

static uint8_t aaceld_debug_count = 0;

static void a2dp_demo_send_media_packet_aaceld(void) {
    // Debug: print first few packets
    // if (aaceld_debug_count < 5) {
    //     printf("AAC-ELD TX: %d bytes, %d frames, ts=%u, first 8: ",
    //            media_tracker.codec_storage_count, media_tracker.codec_num_frames,
    //            (unsigned)media_tracker.rtp_timestamp);
    //     for (int k = 0; k < 8 && k < media_tracker.codec_storage_count; k++) {
    //         printf("%02x ", media_tracker.codec_storage[k]);
    //     }
    //     printf("\n");
    //     aaceld_debug_count++;
    // }

    a2dp_source_stream_send_media_payload_rtp(media_tracker.avdtp_cid, media_tracker.local_seid, 0, media_tracker.rtp_timestamp, &media_tracker.codec_storage[0], media_tracker.codec_storage_count);

    // RTP timestamp: sample-count based (same as standard A2DP)
    uint16_t frames_sent = (media_tracker.codec_num_frames > 0) ? media_tracker.codec_num_frames : 1;
    media_tracker.rtp_timestamp += frames_sent * acc_num_simples;

    media_tracker.codec_storage_count = 0;
    media_tracker.codec_ready_to_send = 0;
    media_tracker.codec_num_frames = 0;
}

static void a2dp_demo_send_media_packet_ldac(void) {

    uint8_t num_frames = media_tracker.codec_num_frames;
    media_tracker.codec_storage[0] = num_frames; // frames in first byte

    a2dp_source_stream_send_media_payload_rtp(media_tracker.avdtp_cid, media_tracker.local_seid, 0, media_tracker.rtp_timestamp, &media_tracker.codec_storage[0], media_tracker.codec_storage_count);
    media_tracker.rtp_timestamp += num_frames * LDACBT_ENC_LSU;

    media_tracker.codec_storage_count = 0;
    media_tracker.codec_ready_to_send = 0;
    media_tracker.codec_num_frames = 0;
}


static uint32_t get_vendor_id(const uint8_t *codec_info) {
    uint32_t vendor_id = 0;
    vendor_id |= codec_info[0];
    vendor_id |= codec_info[1] << 8;
    vendor_id |= codec_info[2] << 16;
    vendor_id |= codec_info[3] << 24;
    return vendor_id;
}

static uint16_t get_codec_id(const uint8_t *codec_info) {
    uint16_t codec_id = 0;
    codec_id |= codec_info[4];
    codec_id |= codec_info[5] << 8;
    return codec_id;
}

static void a2dp_demo_send_media_packet(void) {
    adtvp_media_codec_capabilities_t local_cap;
    switch (remote_seps[selected_remote_sep_index].sep.capabilities.media_codec.media_codec_type){
        case AVDTP_CODEC_SBC:
            a2dp_demo_send_media_packet_sbc();
            break;

        case AVDTP_CODEC_MPEG_2_4_AAC:
            a2dp_demo_send_media_packet_aac();
            break;

        case AVDTP_CODEC_NON_A2DP:
            local_cap = sc.local_stream_endpoint->sep.capabilities.media_codec;
            uint32_t local_vendor_id = get_vendor_id(local_cap.media_codec_information);
            uint16_t local_codec_id = get_codec_id(local_cap.media_codec_information);
            if (local_vendor_id == A2DP_CODEC_VENDOR_ID_SONY && local_codec_id == A2DP_SONY_CODEC_LDAC)
                a2dp_demo_send_media_packet_ldac();
            else if (local_vendor_id == A2DP_CODEC_VENDOR_ID_APPLE && local_codec_id == A2DP_APPLE_CODEC_AAC_ELD)
                a2dp_demo_send_media_packet_aaceld();
            break;
        default:
            // TODO:
            printf("Send media payload for %s not implemented yet\n", codec_name_for_type(sc.local_stream_endpoint->media_codec_type));
            break;
    }
}



// --- Slot queue audio buffer ---
typedef struct {
    int16_t data[AUDIO_SLOT_MAX_INT16];
} audio_slot_t;

static audio_slot_t slot_pool[AUDIO_SLOT_COUNT_MAX];
static queue_t free_queue;
static queue_t ready_queue;
static uint8_t active_slot_count = AUDIO_SLOT_COUNT_SBC; // current queue depth

// USB-side filling state (written only from Core 0)
static uint8_t  filling_slot_idx;
static uint16_t filling_offset    = 0;
static bool     filling_slot_valid = false;
static uint16_t slot_frame_int16  = AUDIO_SLOT_MAX_INT16; // set per codec

static bool is_usb_streaming = false;
static bool have_ldac_codec_capabilities = false;
static bool have_aaceld_codec_capabilities = false;
bd_addr_t cur_active_device;

// --- AAC-ELD Core 1 encoder infrastructure ---
#define ENCODED_FRAME_MAX_SIZE 500
#define ENCODED_FRAME_QUEUE_DEPTH 8

typedef struct {
    uint8_t  data[ENCODED_FRAME_MAX_SIZE];  // Apple header + raw AU
    uint16_t length;
} encoded_frame_t;

static encoded_frame_t encoded_frame_pool[ENCODED_FRAME_QUEUE_DEPTH];
static queue_t encoded_free_queue;   // Core 1 gets empty frames from here
static queue_t encoded_ready_queue;  // Core 1 pushes encoded frames here, Core 0 pops

static volatile bool core1_encoder_active = false;
static volatile uint32_t core1_encode_count = 0;  // debug: frames encoded by Core 1
static volatile uint32_t core1_state = 0;          // debug: 0=idle, 1=active, 2=got_raw, 3=got_enc, 4=encoding, 5=encoded
static uint16_t aaceld_frame_sequence = 1;  // used by Core 1 only
bool finish_setup_aac = false;
bool allow_switch_slot = true;

// --- Suspend recovery: restart stream if suspend response never arrives ---
static btstack_timer_source_t suspend_recovery_timer;
static bool suspend_recovery_pending = false;
#define SUSPEND_RECOVERY_TIMEOUT_MS 3000

static void a2dp_demo_timer_start(a2dp_media_sending_context_t * context);

static void suspend_recovery_handler(btstack_timer_source_t *timer) {
    (void)timer;
    if (!suspend_recovery_pending) return;
    suspend_recovery_pending = false;

    printf("Suspend recovery: no response after %d ms, restarting stream\n", SUSPEND_RECOVERY_TIMEOUT_MS);
    // Try to start the stream directly (skip suspend since it timed out)
    avdtp_source_start_stream(media_tracker.avdtp_cid, media_tracker.local_seid);
}

void audio_slot_queue_init(void) {
    active_slot_count = AUDIO_SLOT_COUNT_SBC;  // default
    queue_init(&free_queue,  sizeof(uint8_t), AUDIO_SLOT_COUNT_MAX);
    queue_init(&ready_queue, sizeof(uint8_t), AUDIO_SLOT_COUNT_MAX);
    for (uint8_t i = 0; i < active_slot_count; i++) {
        queue_try_add(&free_queue, &i);
    }
    filling_slot_valid = false;
    filling_offset = 0;
}

void audio_slot_queue_configure_with_count(uint16_t samples_per_slot, uint8_t slot_count) {
    // Drain both queues
    uint8_t idx;
    while (queue_try_remove(&ready_queue, &idx)) {}
    while (queue_try_remove(&free_queue, &idx)) {}
    // Return filling slot if any
    if (filling_slot_valid) {
        filling_slot_valid = false;
    }
    filling_offset = 0;
    slot_frame_int16 = samples_per_slot * 2; // stereo
    active_slot_count = (slot_count <= AUDIO_SLOT_COUNT_MAX) ? slot_count : AUDIO_SLOT_COUNT_MAX;
    // Repopulate free queue with new count
    for (uint8_t i = 0; i < active_slot_count; i++) {
        queue_try_add(&free_queue, &i);
    }
    printf("Slot queue: %d slots x %d samples\n", active_slot_count, samples_per_slot);
}

void audio_slot_queue_configure(uint16_t samples_per_slot) {
    audio_slot_queue_configure_with_count(samples_per_slot, active_slot_count);
}

void audio_slot_push_samples(const int16_t *src, uint16_t stereo_pair_count) {
    uint16_t remaining = stereo_pair_count * 2; // total int16_t values
    uint16_t src_offset = 0;

    while (remaining > 0) {
        // Acquire a filling slot if we don't have one
        if (!filling_slot_valid) {
            if (!queue_try_remove(&free_queue, &filling_slot_idx)) {
                return; // no free slots, drop samples
            }
            filling_slot_valid = true;
            filling_offset = 0;
        }

        // Copy as much as fits into the current slot
        uint16_t space = slot_frame_int16 - filling_offset;
        uint16_t to_copy = (remaining < space) ? remaining : space;
        memcpy(&slot_pool[filling_slot_idx].data[filling_offset],
               &src[src_offset], to_copy * sizeof(int16_t));
        filling_offset += to_copy;
        src_offset += to_copy;
        remaining -= to_copy;

        // If slot is full, push to ready queue
        if (filling_offset >= slot_frame_int16) {
            if (!queue_try_add(&ready_queue, &filling_slot_idx)) {
                // Ready queue full, return slot to free (drop data)
                queue_try_add(&free_queue, &filling_slot_idx);
            }
            filling_slot_valid = false;
            filling_offset = 0;
        }
    }
}

// BT-side helpers: pop a ready slot or get a silence slot
static bool audio_slot_pop(uint8_t *slot_idx) {
    if (queue_try_remove(&ready_queue, slot_idx)) {
        return true;
    }
    // No data ready — produce silence
    if (queue_try_remove(&free_queue, slot_idx)) {
        memset(slot_pool[*slot_idx].data, 0, slot_frame_int16 * sizeof(int16_t));
        return true;
    }
    return false;
}

// Check if real audio data is available (not silence)
static bool audio_slot_has_data(void) {
    return queue_get_level(&ready_queue) > 0;
}

static void audio_slot_release(uint8_t slot_idx) {
    queue_try_add(&free_queue, &slot_idx);
}

// --- Core 1 AAC-ELD encoder ---

static void encoded_frame_queue_init(void) {
    queue_init(&encoded_free_queue,  sizeof(uint8_t), ENCODED_FRAME_QUEUE_DEPTH);
    queue_init(&encoded_ready_queue, sizeof(uint8_t), ENCODED_FRAME_QUEUE_DEPTH);
    for (uint8_t i = 0; i < ENCODED_FRAME_QUEUE_DEPTH; i++) {
        queue_try_add(&encoded_free_queue, &i);
    }
}

void core1_aaceld_encoder_loop(void) {
    // FDK-AAC encoder buffers
    AACENC_BufDesc in_buf = {0}, out_buf = {0};
    AACENC_InArgs  in_args = {0};
    AACENC_OutArgs out_args = {0};
    int in_identifier = IN_AUDIO_DATA;
    int in_size, in_elem_size = 2;
    int out_identifier = OUT_BITSTREAM_DATA;
    int out_size, out_elem_size = 1;
    void *in_ptr, *out_ptr;

    in_buf.numBufs           = 1;
    in_buf.bufs              = &in_ptr;
    in_buf.bufferIdentifiers = &in_identifier;
    in_buf.bufSizes          = &in_size;
    in_buf.bufElSizes        = &in_elem_size;

    out_buf.numBufs           = 1;
    out_buf.bufs              = &out_ptr;
    out_buf.bufferIdentifiers = &out_identifier;
    out_buf.bufSizes          = &out_size;
    out_buf.bufElSizes        = &out_elem_size;

    while (true) {
        if (!core1_encoder_active) {
            core1_state = 0;
            sleep_ms(1);
            continue;
        }
        core1_state = 1;

        // Get a raw audio slot (real data or silence)
        uint8_t raw_slot_idx;
        if (!audio_slot_pop(&raw_slot_idx)) {
            sleep_us(500);
            continue;
        }
        core1_state = 2;

        // Get an encoded frame slot
        uint8_t enc_idx;
        if (!queue_try_remove(&encoded_free_queue, &enc_idx)) {
            // No free encoded slots — drop raw data
            audio_slot_release(raw_slot_idx);
            sleep_us(500);
            continue;
        }
        core1_state = 3;

        // Encode with FDK-AAC
        unsigned int num_samples = acc_num_simples * aacinf.inputChannels;
        in_ptr               = slot_pool[raw_slot_idx].data;
        in_size              = num_samples * sizeof(int16_t);
        in_args.numInSamples = num_samples;

        // Leave 4 bytes at start for Apple header
        out_ptr  = &encoded_frame_pool[enc_idx].data[4];
        out_size = ENCODED_FRAME_MAX_SIZE - 4;

        core1_state = 4;
        AACENC_ERROR err = aacEncEncode(handleAAC, &in_buf, &out_buf, &in_args, &out_args);
        core1_state = 5;

        // Release raw slot immediately
        audio_slot_release(raw_slot_idx);

        if (err != AACENC_OK || out_args.numOutBytes == 0) {
            queue_try_add(&encoded_free_queue, &enc_idx);
            continue;
        }

        core1_encode_count++;

        // Prepend Apple AAC-ELD Size Header
        uint16_t seq = aaceld_frame_sequence;
        aaceld_frame_sequence = (aaceld_frame_sequence + 1) & 0x0FFF;
        uint16_t au_size = out_args.numOutBytes;
        encoded_frame_pool[enc_idx].data[0] = 0xB0 | ((seq >> 8) & 0x0F);
        encoded_frame_pool[enc_idx].data[1] = seq & 0xFF;
        encoded_frame_pool[enc_idx].data[2] = 0x10 | ((au_size >> 8) & 0x0F);
        encoded_frame_pool[enc_idx].data[3] = au_size & 0xFF;
        encoded_frame_pool[enc_idx].length = 4 + out_args.numOutBytes;

        // Push to ready queue for Core 0
        if (!queue_try_add(&encoded_ready_queue, &enc_idx)) {
            queue_try_add(&encoded_free_queue, &enc_idx);
        }
    }
}

bool check_is_streaming(){
    return is_streaming;
}

void set_usb_streaming(bool flag){
    is_usb_streaming = flag;
}

bool get_allow_switch_slot(){
    return allow_switch_slot;
}


static int fill_sbc_audio_buffer(a2dp_media_sending_context_t * context){
    int total_num_bytes_read = 0;
    unsigned int num_audio_samples_per_sbc_buffer = btstack_sbc_encoder_num_audio_frames();

    while (context->samples_ready >= num_audio_samples_per_sbc_buffer &&
           (context->max_media_payload_size - context->codec_storage_count) >= btstack_sbc_encoder_sbc_buffer_length()){

        uint8_t slot_idx;
        if (!audio_slot_pop(&slot_idx)) break;

        btstack_sbc_encoder_process_data(slot_pool[slot_idx].data);

        uint16_t sbc_frame_size = btstack_sbc_encoder_sbc_buffer_length();
        uint8_t * sbc_frame = btstack_sbc_encoder_sbc_buffer();

        total_num_bytes_read += num_audio_samples_per_sbc_buffer;

        memcpy(&context->codec_storage[1 + context->codec_storage_count], sbc_frame, sbc_frame_size);
        context->codec_storage_count += sbc_frame_size;
        context->samples_ready -= num_audio_samples_per_sbc_buffer;

        audio_slot_release(slot_idx);
    }

    return total_num_bytes_read;
}

static int a2dp_demo_fill_ldac_audio_buffer(a2dp_media_sending_context_t *context) {
    int          total_samples_read                = 0;
    unsigned int num_audio_samples_per_ldac_buffer = LDACBT_ENC_LSU;
    int          consumed;
    int          encoded = 0;
    int          frames;

    // reserve first byte for number of frames
    if (context->codec_storage_count == 0)
        context->codec_storage_count = 1;

    while (context->samples_ready >= num_audio_samples_per_ldac_buffer && encoded == 0) {

        uint8_t slot_idx;
        if (!audio_slot_pop(&slot_idx)) break;

        if (ldacBT_encode(handleLDAC, slot_pool[slot_idx].data, &consumed, &context->codec_storage[context->codec_storage_count], &encoded, &frames) != 0) {
            printf("LDAC encoding error: %d\n", ldacBT_get_error_code(handleLDAC));
        }
        consumed = consumed / (2 * ldac_configuration.num_channels);
        total_samples_read += consumed;
        context->codec_storage_count += encoded;
        context->codec_num_frames += frames;
        context->samples_ready -= consumed;

        audio_slot_release(slot_idx);
    }

    return total_samples_read;
}


#ifdef HAVE_AAC_FDK

static int fill_aac_audio_buffer(a2dp_media_sending_context_t *context) {
    int          total_samples_read               = 0;

    unsigned int num_audio_samples_per_aac_buffer = acc_num_simples;

    btstack_assert(num_audio_samples_per_aac_buffer <= AUDIO_SLOT_MAX_SAMPLES);

    unsigned required_samples = num_audio_samples_per_aac_buffer * aacinf.inputChannels;

    AACENC_BufDesc in_buf   = { 0 }, out_buf = { 0 };
    AACENC_InArgs  in_args  = { 0 };
    AACENC_OutArgs out_args = { 0 };
    int in_identifier = IN_AUDIO_DATA;
    int in_size, in_elem_size;
    int out_identifier = OUT_BITSTREAM_DATA;
    int out_size, out_elem_size;
    void *in_ptr, *out_ptr;

    in_elem_size             = 2;
    in_buf.numBufs           = 1;
    in_buf.bufs              = &in_ptr;
    in_buf.bufferIdentifiers = &in_identifier;
    in_buf.bufSizes          = &in_size;
    in_buf.bufElSizes        = &in_elem_size;

    out_elem_size             = 1;
    out_buf.numBufs           = 1;
    out_buf.bufs              = &out_ptr;
    out_buf.bufferIdentifiers = &out_identifier;
    out_buf.bufSizes          = &out_size;
    out_buf.bufElSizes        = &out_elem_size;

    // AAC-ELD: Apple per-frame header (BTAudioHALPlugin adds "AAC-ELD Size Header")
    int aaceld_hdr_size = (cur_codec == 4) ? 4 : 0;

    // For AAC-ELD, reserve enough space for a full frame (~350 bytes + header)
    int min_space = (cur_codec == 4) ? 400 : (aaceld_hdr_size + 50);
    while (context->samples_ready >= num_audio_samples_per_aac_buffer &&
           (context->max_media_payload_size - context->codec_storage_count) > min_space) {

        uint8_t slot_idx;
        if (!audio_slot_pop(&slot_idx)) break;

        int hdr_offset = context->codec_storage_count + aaceld_hdr_size;

        in_ptr               = slot_pool[slot_idx].data;
        in_size              = required_samples * sizeof(int16_t);
        in_args.numInSamples = required_samples;
        out_ptr              = &context->codec_storage[hdr_offset];
        out_size             = sizeof(context->codec_storage) - hdr_offset;
        out_buf.bufs         = &out_ptr;
        out_buf.bufSizes     = &out_size;
        AACENC_ERROR err;

        if ((err = aacEncEncode(handleAAC, &in_buf, &out_buf, &in_args, &out_args)) != AACENC_OK) {
            printf("Error in AAC encoding %d, storage=%d/%d, out_size=%d\n",
                   err, context->codec_storage_count, (int)sizeof(context->codec_storage), out_size);
            audio_slot_release(slot_idx);
            break;
        }

        // Add Apple AAC-ELD Size Header per frame
        if (cur_codec == 4 && out_args.numOutBytes > 0) {
            uint16_t seq = aaceld_frame_sequence;
            aaceld_frame_sequence = (aaceld_frame_sequence + 1) & 0x0FFF;  // 12-bit wrap
            uint16_t au_size = out_args.numOutBytes;
            context->codec_storage[context->codec_storage_count + 0] = 0xB0 | ((seq >> 8) & 0x0F);
            context->codec_storage[context->codec_storage_count + 1] = seq & 0xFF;
            context->codec_storage[context->codec_storage_count + 2] = 0x10 | ((au_size >> 8) & 0x0F);
            context->codec_storage[context->codec_storage_count + 3] = au_size & 0xFF;
            context->codec_storage_count += 4 + out_args.numOutBytes;
        } else {
            context->codec_storage_count += out_args.numOutBytes;
        }

        total_samples_read += num_audio_samples_per_aac_buffer;
        context->codec_num_frames++;
        context->samples_ready -= num_audio_samples_per_aac_buffer;

        audio_slot_release(slot_idx);

        // AAC-ELD: max 3 frames per RTP packet
        if (cur_codec == 4 && context->codec_num_frames >= 3) break;
    }
    return total_samples_read;
}

#endif

static void a2dp_demo_timer_pause(a2dp_media_sending_context_t * context);
static void a2dp_demo_timer_stop(a2dp_media_sending_context_t * context);

static void avdtp_audio_timeout_handler(btstack_timer_source_t * timer){

    adtvp_media_codec_capabilities_t local_cap;
    a2dp_media_sending_context_t * context = (a2dp_media_sending_context_t *) btstack_run_loop_get_timer_context(timer);
    btstack_run_loop_set_timer(&context->audio_timer, audio_timer_interval);
    btstack_run_loop_add_timer(&context->audio_timer);
    uint32_t now = btstack_run_loop_get_time_ms();

    uint32_t update_period_ms = audio_timer_interval;
    if (context->time_audio_data_sent > 0){
        update_period_ms = now - context->time_audio_data_sent;
    }

    //printf("update_period_ms is %d\n", update_period_ms);
    //printf("a2dp_sample_rate() is %d\n", a2dp_sample_rate());


    uint32_t num_samples = (update_period_ms * a2dp_sample_rate()) / 1000;
    context->acc_num_missed_samples += (update_period_ms * a2dp_sample_rate()) % 1000;
    
    while (context->acc_num_missed_samples >= 1000){
        num_samples++;
        context->acc_num_missed_samples -= 1000;
    }
    context->time_audio_data_sent = now;
    context->samples_ready += num_samples;

    // Cap to prevent unbounded accumulation when USB audio pauses
    uint32_t max_ready = 4 * acc_num_simples;
    if (context->samples_ready > max_ready) {
        context->samples_ready = max_ready;
    }

    //printf("num_samples is %d\n", num_samples);
    //printf("samples_ready is %d\n", context->samples_ready);

    {
        static uint32_t send_wait_ticks = 0;
        static uint32_t fail_count = 0;
        if (context->codec_ready_to_send) {
            send_wait_ticks++;
            if (send_wait_ticks > (100 / audio_timer_interval)) {
                // Can't send for 100ms — drop data and retry
                context->codec_ready_to_send = 0;
                context->codec_storage_count = 0;
                context->codec_num_frames = 0;
                context->samples_ready = 0;
                send_wait_ticks = 0;
                fail_count++;

                // AAC-ELD: suspend immediately on first failure to reset AirPods decoder.
                // For other codecs, wait for 3 failures.
                int suspend_threshold = (cur_codec == 4) ? 1 : 3;

                if (fail_count >= suspend_threshold) {
                    printf("Signal bad suspending stream for recovery (codec=%d, fail=%d)\n",
                           cur_codec, fail_count);

                    // Reset AAC-ELD encoder + sequence so both sides start fresh
                    if (cur_codec == 4) {
                        aaceld_frame_sequence = 1;
                        context->rtp_timestamp = 0;

                        // Reset FDK-AAC encoder to flush internal overlap state
                        if (handleAAC != NULL) {
                            aacEncClose(&handleAAC);
                            handleAAC = NULL;
                        }
                        AACENC_ERROR err;
                        if ((err = aacEncOpen(&handleAAC, 0x01, 2)) == AACENC_OK) {
                            aacEncoder_SetParam(handleAAC, AACENC_AOT, 39);
                            aacEncoder_SetParam(handleAAC, AACENC_BITRATE, 265000);
                            aacEncoder_SetParam(handleAAC, AACENC_SAMPLERATE, 48000);
                            aacEncoder_SetParam(handleAAC, AACENC_CHANNELMODE, 2);
                            aacEncoder_SetParam(handleAAC, AACENC_GRANULE_LENGTH, 480);
                            aacEncoder_SetParam(handleAAC, AACENC_SBR_MODE, 0);
                            aacEncoder_SetParam(handleAAC, AACENC_BITRATEMODE, 0);
                            aacEncoder_SetParam(handleAAC, AACENC_AFTERBURNER, 0);
                            aacEncoder_SetParam(handleAAC, AACENC_TRANSMUX, TT_MP4_RAW);
                            aacEncEncode(handleAAC, NULL, NULL, NULL, NULL);
                            aacEncInfo(handleAAC, &aacinf);
                        }
                        printf("AAC-ELD: encoder reset, seq/ts/state cleared\n");
                    }

                    a2dp_demo_timer_stop(context);
                    int suspend_status = avdtp_source_suspend(context->avdtp_cid, context->local_seid);
                    fail_count = 0;

                    if (suspend_status != ERROR_CODE_SUCCESS) {
                        // Suspend failed immediately — restart timer directly
                        printf("Suspend failed (status %d), restarting timer\n", suspend_status);
                        a2dp_demo_timer_start(context);
                    } else {
                        // Arm recovery timer in case suspend response never arrives
                        suspend_recovery_pending = true;
                        btstack_run_loop_remove_timer(&suspend_recovery_timer);
                        btstack_run_loop_set_timer_handler(&suspend_recovery_timer, suspend_recovery_handler);
                        btstack_run_loop_set_timer(&suspend_recovery_timer, SUSPEND_RECOVERY_TIMEOUT_MS);
                        btstack_run_loop_add_timer(&suspend_recovery_timer);
                    }
                    return;
                }
            }
            return;
        }
        send_wait_ticks = 0;
        fail_count = 0;  // reset on successful send
    }

    avdtp_media_codec_type_t codec_type = sc.local_stream_endpoint->remote_configuration.media_codec.media_codec_type;

    switch (codec_type){

        case AVDTP_CODEC_SBC:
            fill_sbc_audio_buffer(context);
            if ((context->codec_storage_count + btstack_sbc_encoder_sbc_buffer_length()) > context->max_media_payload_size){
                // schedule sending
                context->codec_ready_to_send = 1;
                a2dp_source_stream_endpoint_request_can_send_now(context->avdtp_cid, context->local_seid);
            }
            break;
        case AVDTP_CODEC_MPEG_1_2_AUDIO:
            break;
        case AVDTP_CODEC_ATRAC_FAMILY:
            break;
#ifdef HAVE_AAC_FDK
        case AVDTP_CODEC_MPEG_2_4_AAC:
            if (context->codec_storage_count == 0){
                fill_aac_audio_buffer(context);
            }

            if (context->codec_storage_count > 0) {
                // schedule sending
                context->codec_ready_to_send = 1;
                a2dp_source_stream_endpoint_request_can_send_now(context->avdtp_cid, context->local_seid);
            }
            break;
#endif
        case AVDTP_CODEC_NON_A2DP:
            local_cap = sc.local_stream_endpoint->sep.capabilities.media_codec;
            uint32_t local_vendor_id = get_vendor_id(local_cap.media_codec_information);
            uint16_t local_codec_id = get_codec_id(local_cap.media_codec_information);

            // LDAC
            if (local_vendor_id == A2DP_CODEC_VENDOR_ID_SONY && local_codec_id == A2DP_SONY_CODEC_LDAC) {
                if (context->codec_ready_to_send)
                    return;
                a2dp_demo_fill_ldac_audio_buffer(context);

                if (context->codec_storage_count > 1) {
                    context->codec_ready_to_send = 1;
                    a2dp_source_stream_endpoint_request_can_send_now(context->avdtp_cid, context->local_seid);
                }
            }
            // Apple AAC-ELD: encode on Core 0 (FDK-AAC not safe for Core 1)
            else if (local_vendor_id == A2DP_CODEC_VENDOR_ID_APPLE && local_codec_id == A2DP_APPLE_CODEC_AAC_ELD) {
                fill_aac_audio_buffer(context);

                if (context->codec_num_frames >= 3 ||
                    (context->codec_num_frames > 0 &&
                     (context->max_media_payload_size - context->codec_storage_count) <= 400)) {
                    context->codec_ready_to_send = 1;
                    a2dp_source_stream_endpoint_request_can_send_now(context->avdtp_cid, context->local_seid);
                }
            }
            break;
        default:
            break;
    }
}


static void a2dp_demo_timer_start(a2dp_media_sending_context_t * context){

    // AAC-ELD: max 986 bytes per RTP payload (Apple negotiated limit)
    // AAC-LC/SBC/LDAC: 656 bytes
    if (cur_codec == 4) {
        context->max_media_payload_size = 986;
    } else {
        context->max_media_payload_size = 0x290;
    }
    context->codec_storage_count = 0;
    context->codec_ready_to_send = 0;
    context->streaming = 1;
    btstack_run_loop_remove_timer(&context->audio_timer);
    btstack_run_loop_set_timer_handler(&context->audio_timer, avdtp_audio_timeout_handler);
    btstack_run_loop_set_timer_context(&context->audio_timer, context);
    btstack_run_loop_set_timer(&context->audio_timer, audio_timer_interval);
    btstack_run_loop_add_timer(&context->audio_timer);
}

static void a2dp_demo_timer_stop(a2dp_media_sending_context_t * context){
    core1_encoder_active = false;  // stop Core 1 encoder
    suspend_recovery_pending = false;
    btstack_run_loop_remove_timer(&suspend_recovery_timer);
    context->time_audio_data_sent = 0;
    context->acc_num_missed_samples = 0;
    context->samples_ready = 0;
    context->streaming = 1;
    context->codec_storage_count = 0;
    context->codec_ready_to_send = 0;
    btstack_run_loop_remove_timer(&context->audio_timer);
}

static void a2dp_demo_timer_pause(a2dp_media_sending_context_t * context){
    is_streaming = false;
    context->time_audio_data_sent = 0;
    context->acc_num_missed_samples = 0;
    context->samples_ready = 0;
    context->streaming = 1;
    context->codec_storage_count = 0;
    context->codec_ready_to_send = 0;
    btstack_run_loop_remove_timer(&context->audio_timer);
} 


static void dump_sbc_capability(media_codec_information_sbc_t media_codec_sbc){
    printf("    - sampling_frequency: 0x%02x\n", media_codec_sbc.sampling_frequency_bitmap);
    printf("    - channel_mode: 0x%02x\n", media_codec_sbc.channel_mode_bitmap);
    printf("    - block_length: 0x%02x\n", media_codec_sbc.block_length_bitmap);
    printf("    - subbands: 0x%02x\n", media_codec_sbc.subbands_bitmap);
    printf("    - allocation_method: 0x%02x\n", media_codec_sbc.allocation_method_bitmap);
    printf("    - bitpool_value [%d, %d] \n", media_codec_sbc.min_bitpool_value, media_codec_sbc.max_bitpool_value);
}

static void dump_sbc_configuration(avdtp_media_codec_configuration_sbc_t configuration){
    printf("Received media codec configuration:\n");
    printf("    - num_channels: %d\n", configuration.num_channels);
    printf("    - sampling_frequency: %d\n", configuration.sampling_frequency);
    printf("    - channel_mode: %d\n", configuration.channel_mode);
    printf("    - block_length: %d\n", configuration.block_length);
    printf("    - subbands: %d\n", configuration.subbands);
    printf("    - allocation_method: %d\n", configuration.allocation_method);
    printf("    - bitpool_value [%d, %d] \n", configuration.min_bitpool_value, configuration.max_bitpool_value);
}

static void dump_remote_sink_endpoints(void){
    printf("Remote Endpoints:\n");
    int i;
    for (i=0;i<num_remote_seps;i++) {
        printf("- %u. remote seid %u\n", i, remote_seps[i].sep.seid);
    }
}

static int find_remote_seid(uint8_t remote_seid){
    int i;
    for (i=0;i<num_remote_seps;i++){
        if (remote_seps[i].sep.seid == remote_seid){
            return i;
        }
    }
    return -1;
}



#ifdef HAVE_AAC_FDK
static int convert_aac_object_type(int bitmap) {
    switch (bitmap) {
        case AVDTP_AAC_MPEG4_SCALABLE:
            return AOT_AAC_SCAL;
        case AVDTP_AAC_MPEG4_LTP:
            return AOT_AAC_LTP;
        case AVDTP_AAC_MPEG4_LC:
            return AOT_AAC_LC;
        case AVDTP_AAC_MPEG2_LC:
            // https://lists.freedesktop.org/archives/gstreamer-commits/2016-September/096332.html
            return AOT_AAC_LC;
        default:
            printf("invalid aac aot config %d\n", bitmap);
            return 0;
    }
}

static int convert_aac_vbr(int vbr) {
    if (vbr)
        return 4;
    else
        return 0;
}
#endif

static int convert_ldac_sampling_frequency(uint8_t frequency_bitmap) {
    switch (frequency_bitmap) {
    case 1 << 0:
        return 192000;
    case 1 << 1:
        return 176400;
    case 1 << 2:
        return 96000;
    case 1 << 3:
        return 88200;
    case 1 << 4:
        return 48000;
    case 1 << 5:
        return 44100;
    default:
        printf("invalid ldac sampling frequency %d\n", frequency_bitmap);
        return 0;
    }
}

static int convert_ldac_num_channels(uint8_t channel_mode) {
    switch (channel_mode) {
    case 1 << 0: // stereo
    case 1 << 1: // dual channel
        return 2;
    case 1 << 2:
        return 1;
    default:
        printf("error ldac channel mode\n");
        return 0;
    }
}

static void packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size){
    if (packet_type != HCI_EVENT_PACKET) return;
    if (hci_event_packet_get_type(packet) != HCI_EVENT_AVDTP_META) return;
    UNUSED(channel);
    UNUSED(size);

    uint8_t signal_identifier;
    uint8_t status;
    avdtp_sep_t sep;
    uint16_t avdtp_cid;
    uint8_t remote_seid;
    int local_remote_seid_index;
    uint32_t vendor_id;
    uint16_t codec_id;
    bd_addr_t address;


    //printf("Current packet event is 0x%02x\n", packet[2]);

    switch (packet[2]){
        case AVDTP_SUBEVENT_SIGNALING_CONNECTION_ESTABLISHED:
            avdtp_subevent_signaling_connection_established_get_bd_addr(packet, address);
            // print info
            printf("Current Device found: %s \n",  bd_addr_to_str(address));
            memcpy(cur_active_device, address, sizeof(bd_addr_t));

            avdtp_cid = avdtp_subevent_signaling_connection_established_get_avdtp_cid(packet);
            status = avdtp_subevent_signaling_connection_established_get_status(packet);
            if (status != 0){
                printf("AVDTP source signaling connection failed: status %d\n", status);
                break;
            }
            media_tracker.avdtp_cid = avdtp_subevent_signaling_connection_established_get_avdtp_cid(packet);
            printf("AVDTP source signaling connection established: avdtp_cid 0x%02x\n", avdtp_cid);

            allow_switch_slot = false;
            set_led_mode_off();
            // seid selected per argv
            num_remote_seps = 0;
            selected_remote_sep_index = 0;
            have_ldac_codec_capabilities = false;
            have_aaceld_codec_capabilities = false;
            finish_setup_aac = false;
            status = avdtp_source_discover_stream_endpoints(media_tracker.avdtp_cid);
            a2dp_is_connected_flag = true;

            break;
        
        case AVDTP_SUBEVENT_STREAMING_CONNECTION_ESTABLISHED:
            status = avdtp_subevent_streaming_connection_established_get_status(packet);
            if (status != 0){
                printf("Streaming connection failed: status %d\n", status);
                break;
            }
            avdtp_cid = avdtp_subevent_streaming_connection_established_get_avdtp_cid(packet);
            media_tracker.local_seid = avdtp_subevent_streaming_connection_established_get_local_seid(packet);
            media_tracker.remote_seid = avdtp_subevent_streaming_connection_established_get_remote_seid(packet);
            a2dp_is_connected_flag = true;

            printf("Streaming connection established, avdtp_cid 0x%02x\n", avdtp_cid);

            avdtp_source_start_stream(media_tracker.avdtp_cid, media_tracker.local_seid);
            avrcp_connect((uint8_t *) cur_active_device, &media_tracker.avrcp_cid);
            break;

        case AVDTP_SUBEVENT_SIGNALING_SEP_FOUND:
            memset(&sep, 0, sizeof(avdtp_sep_t));
            sep.seid = avdtp_subevent_signaling_sep_found_get_remote_seid(packet);;
            sep.in_use = avdtp_subevent_signaling_sep_found_get_in_use(packet);
            sep.media_type = avdtp_subevent_signaling_sep_found_get_media_type(packet);
            sep.type = avdtp_subevent_signaling_sep_found_get_sep_type(packet);
            printf("Found sep: seid %u, in_use %d, media type %d, sep type %d (1-SNK)\n", sep.seid, sep.in_use, sep.media_type, sep.type);
            if (sep.type == AVDTP_SINK) {
                remote_seps[num_remote_seps].sep = sep;
                num_remote_seps++;
            }
            break;

        case AVDTP_SUBEVENT_SIGNALING_SEP_DICOVERY_DONE:
            // select remote if there's only a single remote
            if (num_remote_seps == 1){
                media_tracker.remote_seid = remote_seps[0].sep.seid;
                printf("Only one remote Stream Endpoint with SEID %u, select it for initiator commands\n", media_tracker.remote_seid);
            }


            cur_num_remote_seps = 0;
            // find 1st capabilities; should be sbc
            avdtp_source_get_all_capabilities(media_tracker.avdtp_cid, remote_seps[0].sep.seid);

            break;

        case AVDTP_SUBEVENT_SIGNALING_CAPABILITIES_DONE:
            cur_num_remote_seps ++;
            if (cur_num_remote_seps < num_remote_seps){
                printf("\n\n Getting next CAPABILITY \n\n");
                avdtp_source_get_all_capabilities(media_tracker.avdtp_cid, remote_seps[cur_num_remote_seps].sep.seid);
            }else{
                printf("finish scaning CAPABILITIES codecs\n");
                set_next_capablity_and_start_stream();
            }
            break;


        case AVDTP_SUBEVENT_SIGNALING_MEDIA_TRANSPORT_CAPABILITY:
            printf("CAPABILITY - MEDIA_TRANSPORT supported on remote.\n");
            break;
        case AVDTP_SUBEVENT_SIGNALING_REPORTING_CAPABILITY:
            printf("CAPABILITY - REPORTING supported on remote.\n");
            break;
        case AVDTP_SUBEVENT_SIGNALING_RECOVERY_CAPABILITY:
            printf("CAPABILITY - RECOVERY supported on remote: \n");
            printf("    - recovery_type                %d\n", avdtp_subevent_signaling_recovery_capability_get_recovery_type(packet));
            printf("    - maximum_recovery_window_size %d\n", avdtp_subevent_signaling_recovery_capability_get_maximum_recovery_window_size(packet));
            printf("    - maximum_number_media_packets %d\n", avdtp_subevent_signaling_recovery_capability_get_maximum_number_media_packets(packet));
            break;
        case AVDTP_SUBEVENT_SIGNALING_CONTENT_PROTECTION_CAPABILITY:
            printf("CAPABILITY - CONTENT_PROTECTION supported on remote: \n");
            printf("    - cp_type           %d\n", avdtp_subevent_signaling_content_protection_capability_get_cp_type(packet));
            printf("    - cp_type_value_len %d\n", avdtp_subevent_signaling_content_protection_capability_get_cp_type_value_len(packet));
            printf("    - cp_type_value     \'%s\'\n", avdtp_subevent_signaling_content_protection_capability_get_cp_type_value(packet));
            break;
        case AVDTP_SUBEVENT_SIGNALING_MULTIPLEXING_CAPABILITY:
            printf("CAPABILITY - MULTIPLEXING supported on remote: \n");
            printf("    - fragmentation                  %d\n", avdtp_subevent_signaling_multiplexing_capability_get_fragmentation(packet));
            printf("    - transport_identifiers_num      %d\n", avdtp_subevent_signaling_multiplexing_capability_get_transport_identifiers_num(packet));
            printf("    - transport_session_identifier_1 %d\n", avdtp_subevent_signaling_multiplexing_capability_get_transport_session_identifier_1(packet));
            printf("    - transport_session_identifier_2 %d\n", avdtp_subevent_signaling_multiplexing_capability_get_transport_session_identifier_2(packet));
            printf("    - transport_session_identifier_3 %d\n", avdtp_subevent_signaling_multiplexing_capability_get_transport_session_identifier_3(packet));
            printf("    - tcid_1                         %d\n", avdtp_subevent_signaling_multiplexing_capability_get_tcid_1(packet));
            printf("    - tcid_2                         %d\n", avdtp_subevent_signaling_multiplexing_capability_get_tcid_2(packet));
            printf("    - tcid_3                         %d\n", avdtp_subevent_signaling_multiplexing_capability_get_tcid_3(packet));
            break;
        case AVDTP_SUBEVENT_SIGNALING_DELAY_REPORTING_CAPABILITY:
            printf("CAPABILITY - DELAY_REPORTING supported on remote.\n");
            break;
        case AVDTP_SUBEVENT_SIGNALING_DELAY_REPORT:
            printf("DELAY_REPORT received: %d.%0d ms, local seid %d\n", 
                avdtp_subevent_signaling_delay_report_get_delay_100us(packet)/10, avdtp_subevent_signaling_delay_report_get_delay_100us(packet)%10,
                avdtp_subevent_signaling_delay_report_get_local_seid(packet));
            break;
        case AVDTP_SUBEVENT_SIGNALING_HEADER_COMPRESSION_CAPABILITY:
            printf("CAPABILITY - HEADER_COMPRESSION supported on remote: \n");
            printf("    - back_ch   %d\n", avdtp_subevent_signaling_header_compression_capability_get_back_ch(packet));
            printf("    - media     %d\n", avdtp_subevent_signaling_header_compression_capability_get_media(packet));
            printf("    - recovery  %d\n", avdtp_subevent_signaling_header_compression_capability_get_recovery(packet));
            break;

        case AVDTP_SUBEVENT_SIGNALING_MEDIA_CODEC_SBC_CAPABILITY:{
            // cache
            remote_seid = avdtp_subevent_signaling_media_codec_sbc_capability_get_remote_seid(packet);
            local_remote_seid_index = find_remote_seid(remote_seid);
            btstack_assert(local_remote_seid_index >= 0);
            (void) memcpy(remote_seps[local_remote_seid_index].media_codec_event, packet, size);
            remote_seps[local_remote_seid_index].sep.capabilities.media_codec.media_codec_type = AVDTP_CODEC_SBC;
            remote_seps[local_remote_seid_index].have_media_codec_apabilities = true;

            printf("CAPABILITY - MEDIA_CODEC: SBC, remote seid %u: \n", remote_seid);

            media_codec_information_sbc_t   sbc_capability;
            sbc_capability.sampling_frequency_bitmap = avdtp_subevent_signaling_media_codec_sbc_capability_get_sampling_frequency_bitmap(packet);
            sbc_capability.channel_mode_bitmap = avdtp_subevent_signaling_media_codec_sbc_capability_get_channel_mode_bitmap(packet);
            sbc_capability.block_length_bitmap = avdtp_subevent_signaling_media_codec_sbc_capability_get_block_length_bitmap(packet);
            sbc_capability.subbands_bitmap = avdtp_subevent_signaling_media_codec_sbc_capability_get_subbands_bitmap(packet);
            sbc_capability.allocation_method_bitmap = avdtp_subevent_signaling_media_codec_sbc_capability_get_allocation_method_bitmap(packet);
            sbc_capability.min_bitpool_value = avdtp_subevent_signaling_media_codec_sbc_capability_get_min_bitpool_value(packet);
            sbc_capability.max_bitpool_value = avdtp_subevent_signaling_media_codec_sbc_capability_get_max_bitpool_value(packet);
            dump_sbc_capability(sbc_capability);

            break;
        }

        case AVDTP_SUBEVENT_SIGNALING_MEDIA_CODEC_MPEG_AUDIO_CAPABILITY:
            // cache
            remote_seid = avdtp_subevent_signaling_media_codec_sbc_capability_get_remote_seid(packet);
            local_remote_seid_index = find_remote_seid(remote_seid);
            btstack_assert(local_remote_seid_index >= 0);
            (void) memcpy(remote_seps[local_remote_seid_index].media_codec_event, packet, size);
            remote_seps[local_remote_seid_index].sep.capabilities.media_codec.media_codec_type = AVDTP_CODEC_MPEG_1_2_AUDIO;
            remote_seps[local_remote_seid_index].have_media_codec_apabilities = true;
            printf("CAPABILITY - MEDIA_CODEC: MPEG AUDIO, remote seid %u: \n", remote_seid);

            break;

        case AVDTP_SUBEVENT_SIGNALING_MEDIA_CODEC_MPEG_AAC_CAPABILITY: {
            // cache
            remote_seid = avdtp_subevent_signaling_media_codec_sbc_capability_get_remote_seid(packet);
            local_remote_seid_index = find_remote_seid(remote_seid);
            btstack_assert(local_remote_seid_index >= 0);
            (void) memcpy(remote_seps[local_remote_seid_index].media_codec_event, packet, size);
            remote_seps[local_remote_seid_index].sep.capabilities.media_codec.media_codec_type = AVDTP_CODEC_MPEG_2_4_AAC;
            remote_seps[local_remote_seid_index].have_media_codec_apabilities = true;
            printf("CAPABILITY - MEDIA_CODEC: MPEG AAC, remote seid %u: \n", remote_seid);

            avdtp_media_codec_capabilities_aac_t aac_capabilities;
            aac_capabilities.sampling_frequency_bitmap = a2dp_subevent_signaling_media_codec_mpeg_aac_capability_get_sampling_frequency_bitmap(packet);
            aac_capabilities.object_type_bitmap = a2dp_subevent_signaling_media_codec_mpeg_aac_capability_get_object_type_bitmap(packet);
            aac_capabilities.channels_bitmap = a2dp_subevent_signaling_media_codec_mpeg_aac_capability_get_channels_bitmap(packet);
            aac_capabilities.bit_rate = a2dp_subevent_signaling_media_codec_mpeg_aac_capability_get_bit_rate(packet);
            aac_capabilities.vbr = a2dp_subevent_signaling_media_codec_mpeg_aac_capability_get_vbr(packet);
            printf("A2DP Source: Received AAC capabilities! Sampling frequency bitmap: 0x%04x, object type %u, channel mode %u, bitrate %u, vbr: %u\n",
                   aac_capabilities.sampling_frequency_bitmap, aac_capabilities.object_type_bitmap, aac_capabilities.channels_bitmap,
                   aac_capabilities.bit_rate, aac_capabilities.vbr);
            aac_bit_rate = aac_capabilities.bit_rate;
            break;
        }

        case AVDTP_SUBEVENT_SIGNALING_MEDIA_CODEC_ATRAC_CAPABILITY:
            // cache
            remote_seid = avdtp_subevent_signaling_media_codec_sbc_capability_get_remote_seid(packet);
            local_remote_seid_index = find_remote_seid(remote_seid);
            btstack_assert(local_remote_seid_index >= 0);
            (void) memcpy(remote_seps[local_remote_seid_index].media_codec_event, packet, size);
            remote_seps[local_remote_seid_index].sep.capabilities.media_codec.media_codec_type = AVDTP_CODEC_ATRAC_FAMILY;
            remote_seps[local_remote_seid_index].have_media_codec_apabilities = true;
            printf("CAPABILITY - MEDIA_CODEC: ATRAC, remote seid %u: \n", remote_seid);

            break;

        case AVDTP_SUBEVENT_SIGNALING_MEDIA_CODEC_OTHER_CAPABILITY:
            // cache
            remote_seid = avdtp_subevent_signaling_media_codec_sbc_capability_get_remote_seid(packet);
            local_remote_seid_index = find_remote_seid(remote_seid);
            btstack_assert(local_remote_seid_index >= 0);
            (void) memcpy(remote_seps[local_remote_seid_index].media_codec_event, packet, size);
            remote_seps[local_remote_seid_index].sep.capabilities.media_codec.media_codec_type  = AVDTP_CODEC_NON_A2DP;
            remote_seps[local_remote_seid_index].have_media_codec_apabilities = true;
            const uint8_t *media_info = avdtp_subevent_signaling_media_codec_other_capability_get_media_codec_information(packet);
            vendor_id = get_vendor_id(media_info);
            codec_id = get_codec_id(media_info);

            remote_seps[local_remote_seid_index].vendor_id = vendor_id;
            remote_seps[local_remote_seid_index].codec_id = codec_id;

            if (vendor_id == A2DP_CODEC_VENDOR_ID_SONY && codec_id == A2DP_SONY_CODEC_LDAC)
            {
                have_ldac_codec_capabilities = true;
                printf("CAPABILITY - LDAC, remote seid %u\n", remote_seid);
            }
            else if (vendor_id == A2DP_CODEC_VENDOR_ID_APPLE && codec_id == A2DP_APPLE_CODEC_AAC_ELD)
            {
                have_aaceld_codec_capabilities = true;
                printf("CAPABILITY - Apple AAC-ELD, remote seid %u\n", remote_seid);
                uint16_t info_len = avdtp_subevent_signaling_media_codec_other_capability_get_media_codec_information_len(packet);
                printf("  raw bytes (%u): ", info_len);
                for (int k = 0; k < info_len; k++) {
                    printf("%02x ", media_info[k]);
                }
                printf("\n");
            }
            else {
                printf("CAPABILITY - MEDIA_CODEC: OTHER, remote seid %u: \n", remote_seid);
                printf("vendor_id 0x%04x: \n", vendor_id);
                printf("codec_id: 0x%04x: \n", codec_id);
            }

            break;

        case AVDTP_SUBEVENT_SIGNALING_MEDIA_CODEC_SBC_CONFIGURATION:{
            printf("Set configuration and init encoder\n");
            avdtp_media_codec_configuration_sbc_t sbc_configuration;
            sbc_configuration.reconfigure = avdtp_subevent_signaling_media_codec_sbc_configuration_get_reconfigure(packet);
            sbc_configuration.num_channels = avdtp_subevent_signaling_media_codec_sbc_configuration_get_num_channels(packet);
            sbc_configuration.sampling_frequency = avdtp_subevent_signaling_media_codec_sbc_configuration_get_sampling_frequency(packet);
            sbc_configuration.block_length = avdtp_subevent_signaling_media_codec_sbc_configuration_get_block_length(packet);
            sbc_configuration.subbands = avdtp_subevent_signaling_media_codec_sbc_configuration_get_subbands(packet);
            sbc_configuration.min_bitpool_value = avdtp_subevent_signaling_media_codec_sbc_configuration_get_min_bitpool_value(packet);
            sbc_configuration.max_bitpool_value = avdtp_subevent_signaling_media_codec_sbc_configuration_get_max_bitpool_value(packet);
            
            avdtp_channel_mode_t channel_mode = (avdtp_channel_mode_t) avdtp_subevent_signaling_media_codec_sbc_configuration_get_channel_mode(packet);
            avdtp_sbc_allocation_method_t allocation_method = (avdtp_sbc_allocation_method_t) avdtp_subevent_signaling_media_codec_sbc_configuration_get_allocation_method(packet);
            
            // Map Bluetooth spec definition to SBC Encoder expected input
            sbc_configuration.allocation_method = (btstack_sbc_allocation_method_t)(((uint8_t) allocation_method) - 1);
            switch (channel_mode){
                case AVDTP_CHANNEL_MODE_JOINT_STEREO:
                    sbc_configuration.channel_mode = SBC_CHANNEL_MODE_JOINT_STEREO;
                    sbc_configuration.num_channels = 2;
                    break;
                case AVDTP_CHANNEL_MODE_STEREO:
                    sbc_configuration.channel_mode = SBC_CHANNEL_MODE_STEREO;
                    sbc_configuration.num_channels = 2;
                    break;
                case AVDTP_CHANNEL_MODE_DUAL_CHANNEL:
                    sbc_configuration.channel_mode = SBC_CHANNEL_MODE_DUAL_CHANNEL;
                    sbc_configuration.num_channels = 2;
                    break;
                case AVDTP_CHANNEL_MODE_MONO:
                    sbc_configuration.channel_mode = SBC_CHANNEL_MODE_MONO;
                    sbc_configuration.num_channels = 1;
                    break;
                default:
                    btstack_assert(false);
                    break;
            }
            dump_sbc_configuration(sbc_configuration);

            configure_sample_rate(sc.sampling_frequency);
            btstack_sbc_encoder_init(&sbc_encoder_state, SBC_MODE_STANDARD, 
                sbc_configuration.block_length, sbc_configuration.subbands, 
                sbc_configuration.allocation_method, sbc_configuration.sampling_frequency, 
                sbc_configuration.max_bitpool_value,
                sbc_configuration.channel_mode);

            audio_timer_interval = 10;

            audio_slot_queue_configure_with_count(btstack_sbc_encoder_num_audio_frames(), AUDIO_SLOT_COUNT_SBC);

            cur_codec = 1;

            avdtp_source_open_stream(media_tracker.avdtp_cid, media_tracker.local_seid, media_tracker.remote_seid);
            break;
        }

        case AVDTP_SUBEVENT_SIGNALING_MEDIA_CODEC_MPEG_AAC_CONFIGURATION: {
            printf("Set configuration and init encoder\n");

            if(!finish_setup_aac){
                break;
            }
            avdtp_configuration_mpeg_aac_t aac_configuration;
            // avdtp_media_codec_configuration_aac_t aac_configuration;
            aac_configuration.sampling_frequency = a2dp_subevent_signaling_media_codec_mpeg_aac_configuration_get_sampling_frequency(
                    packet);
            aac_configuration.object_type = a2dp_subevent_signaling_media_codec_mpeg_aac_configuration_get_object_type(
                    packet);
            aac_configuration.channels = a2dp_subevent_signaling_media_codec_mpeg_aac_configuration_get_num_channels(
                    packet);
            aac_configuration.bit_rate = a2dp_subevent_signaling_media_codec_mpeg_aac_configuration_get_bit_rate(
                    packet);
            aac_configuration.vbr = a2dp_subevent_signaling_media_codec_mpeg_aac_configuration_get_vbr(packet);

            printf("A2DP Source: Received AAC configuration! Sampling frequency: %u, object type %u, channel mode %u, bitrate %u, vbr: %u\n",
                   aac_configuration.sampling_frequency, aac_configuration.object_type, aac_configuration.channels,
                   aac_configuration.bit_rate, aac_configuration.vbr);

           int aot = convert_aac_object_type(aac_configuration.object_type);

           int vbr = convert_aac_vbr(aac_configuration.vbr);

           //aac_configuration.bit_rate = aac_bit_rate;

            if (aac_bit_rate > max_aac_bit_rate){
                aac_configuration.bit_rate = max_aac_bit_rate;
            } else {
                aac_configuration.bit_rate = aac_bit_rate;
            }


            //init encoder
            AACENC_ERROR err;
            if ((err = aacEncOpen(&handleAAC, 0x01, aac_configuration.channels)) != AACENC_OK) {
                printf("Couldn't open AAC encoder: %d\n", err);
                break;
            }
            if ((err = aacEncoder_SetParam(handleAAC, AACENC_AOT, 2)) != AACENC_OK) {
                printf("Couldn't set audio object type: %d\n", err);
                break;
            }
            if ((err = aacEncoder_SetParam(handleAAC, AACENC_BITRATE, aac_configuration.bit_rate)) != AACENC_OK) {
                printf("Couldn't set bitrate: %d\n", err);
                break;
            }
            if ((err = aacEncoder_SetParam(handleAAC, AACENC_SAMPLERATE, aac_configuration.sampling_frequency)) !=
                AACENC_OK) {
                printf("Couldn't set sampling rate: %d\n", err);
                break;
            }
            if ((err = aacEncoder_SetParam(handleAAC, AACENC_CHANNELMODE, aac_configuration.channels)) != AACENC_OK) {
                printf("Couldn't set channel mode: %d\n", err);
                break;
            }
            if (aac_configuration.vbr) {
                if ((err = aacEncoder_SetParam(handleAAC, AACENC_BITRATEMODE, vbr)) != AACENC_OK) {
                    printf("Couldn't set VBR bitrate mode %u: %d\n", aac_configuration.vbr, err);
                    break;
                }
            }
            if ((err = aacEncoder_SetParam(handleAAC, AACENC_AFTERBURNER, false)) != AACENC_OK) {
                printf("Couldn't enable afterburner: %d\n", err);
                break;
            }
            if ((err = aacEncoder_SetParam(handleAAC, AACENC_TRANSMUX, TT_MP4_LATM_MCP1)) != AACENC_OK) {
                printf("Couldn't enable LATM transport type: %d\n", err);
                break;
            }
            if ((err = aacEncoder_SetParam(handleAAC, AACENC_HEADER_PERIOD, 1)) != AACENC_OK) {
                printf("Couldn't set LATM header period: %d\n", err);
                break;
            }
            if ((err = aacEncoder_SetParam(handleAAC, AACENC_AUDIOMUXVER, 1)) != AACENC_OK) {
                printf("Couldn't set LATM version: %d\n", err);
                break;
            }
            if ((err = aacEncEncode(handleAAC, NULL, NULL, NULL, NULL)) != AACENC_OK) {
                printf("Couldn't initialize AAC encoder: %d\n", err);
                break;
            }
            if ((err = aacEncInfo(handleAAC, &aacinf)) != AACENC_OK) {
                printf("Couldn't get encoder info: %d\n", err);
                break;
            }
            current_sample_rate = aac_configuration.sampling_frequency;
            printf("AAC setup complete\n", err);

            audio_timer_interval = aac_audio_timer_interval;

            audio_slot_queue_configure_with_count(acc_num_simples, AUDIO_SLOT_COUNT_AAC);

            cur_codec = 2;

            avdtp_source_open_stream(media_tracker.avdtp_cid, media_tracker.local_seid, media_tracker.remote_seid);

            break;
        }

        case AVDTP_SUBEVENT_SIGNALING_MEDIA_CODEC_MPEG_AUDIO_CONFIGURATION:
        case AVDTP_SUBEVENT_SIGNALING_MEDIA_CODEC_ATRAC_CONFIGURATION:
            // TODO: handle other configuration event
            printf("Config not handled for %s\n", codec_name_for_type(remote_seps[selected_remote_sep_index].sep.capabilities.media_codec.media_codec_type));
            break;
        case AVDTP_SUBEVENT_SIGNALING_MEDIA_CODEC_OTHER_CONFIGURATION:
            printf("Received other configuration\n");
            uint8_t *codec_info = (uint8_t *) a2dp_subevent_signaling_media_codec_other_configuration_get_media_codec_information(packet);

            vendor_id = get_vendor_id(codec_info);
            codec_id = get_codec_id(codec_info);

            // LDAC
            if (vendor_id == A2DP_CODEC_VENDOR_ID_SONY && codec_id == A2DP_SONY_CODEC_LDAC) {

                ldac_configuration.reconfigure = a2dp_subevent_signaling_media_codec_other_configuration_get_reconfigure(packet);
                ldac_configuration.sampling_frequency = codec_info[6];
                ldac_configuration.channel_mode = codec_info[7];
                //ldac_configuration.sampling_frequency = 44100;
                ldac_configuration.sampling_frequency = convert_ldac_sampling_frequency(ldac_configuration.sampling_frequency);
                //ldac_configuration.num_channels = convert_ldac_num_channels(ldac_configuration.channel_mode);
                ldac_configuration.num_channels = 2;

                printf("A2DP Source: Received LDAC configuration! Sampling frequency: %d, channel mode: %d channels: %d\n",
                        ldac_configuration.sampling_frequency, ldac_configuration.channel_mode, ldac_configuration.num_channels);

                handleLDAC = ldacBT_get_handle();
                if (handleLDAC == NULL) {
                    printf("Failed to get LDAC handle\n");
                    break;
                }

                // init ldac encoder
                int mtu = 1536; // minimal required mtu
                if (ldacBT_init_handle_encode(handleLDAC, mtu, LDACBT_EQMID_SQ, ldac_configuration.channel_mode,
                            LDACBT_SMPL_FMT_S16, ldac_configuration.sampling_frequency) == -1) {
                    printf("Couldn't initialize LDAC encoder: %d\n", ldacBT_get_error_code(handleLDAC));
                    break;
                }
                // HQ -> audio_timer_interval = 1
                // SQ -> audio_timer_interval <= 5
                // MQ -> audio_timer_interval <= 10
                audio_timer_interval = 1;
                current_sample_rate = ldac_configuration.sampling_frequency;
                printf("current LDAC sampling rate is %d \n", current_sample_rate);

                audio_slot_queue_configure_with_count(LDACBT_ENC_LSU, AUDIO_SLOT_COUNT_LDAC);

                cur_codec = 3;

                avdtp_source_open_stream(media_tracker.avdtp_cid, media_tracker.local_seid, media_tracker.remote_seid);

            }
            // Apple AAC-ELD
            else if (vendor_id == A2DP_CODEC_VENDOR_ID_APPLE && codec_id == A2DP_APPLE_CODEC_AAC_ELD) {
                printf("A2DP Source: Received Apple AAC-ELD configuration!\n");
                printf("  raw config bytes: ");
                uint16_t config_info_len = a2dp_subevent_signaling_media_codec_other_configuration_get_media_codec_information_len(packet);
                for (int k = 0; k < config_info_len; k++) {
                    printf("%02x ", codec_info[k]);
                }
                printf("\n");

                // Init FDK-AAC encoder for AAC-ELD (AOT 39)
                AACENC_ERROR err;
                // Close previous encoder if reconnecting
                if (handleAAC != NULL) {
                    aacEncClose(&handleAAC);
                    handleAAC = NULL;
                }
                if ((err = aacEncOpen(&handleAAC, 0x01, 2)) != AACENC_OK) {
                    printf("Couldn't open AAC-ELD encoder: %d\n", err);
                    break;
                }
                if ((err = aacEncoder_SetParam(handleAAC, AACENC_AOT, 39)) != AACENC_OK) {
                    printf("Couldn't set AAC-ELD AOT: %d\n", err);
                    break;
                }
                // 265kbps CBR, ~240 bytes/frame at 480 samples
                // 3*(4+240) = 732 < 986 MTU, with BT overhead ~220kbps < 256kbps link
                if ((err = aacEncoder_SetParam(handleAAC, AACENC_BITRATE, 265000)) != AACENC_OK) {
                    printf("Couldn't set AAC-ELD bitrate: %d\n", err);
                    break;
                }
                if ((err = aacEncoder_SetParam(handleAAC, AACENC_SAMPLERATE, 48000)) != AACENC_OK) {
                    printf("Couldn't set AAC-ELD sample rate: %d\n", err);
                    break;
                }
                if ((err = aacEncoder_SetParam(handleAAC, AACENC_CHANNELMODE, 2)) != AACENC_OK) {
                    printf("Couldn't set AAC-ELD channel mode: %d\n", err);
                    break;
                }
                // 480 samples = 10ms at 48kHz (AAC-ELD-Stereo48K-10ms mode 130)
                if ((err = aacEncoder_SetParam(handleAAC, AACENC_GRANULE_LENGTH, 480)) != AACENC_OK) {
                    printf("Couldn't set AAC-ELD frame size: %d\n", err);
                    break;
                }
                if ((err = aacEncoder_SetParam(handleAAC, AACENC_SBR_MODE, 0)) != AACENC_OK) {
                    printf("Couldn't set AAC-ELD SBR mode: %d\n", err);
                    break;
                }
                if ((err = aacEncoder_SetParam(handleAAC, AACENC_BITRATEMODE, 0)) != AACENC_OK) {
                    printf("Couldn't set AAC-ELD VBR mode: %d\n", err);
                    break;
                }
                if ((err = aacEncoder_SetParam(handleAAC, AACENC_AFTERBURNER, 0)) != AACENC_OK) {
                    printf("Couldn't disable AAC-ELD afterburner: %d\n", err);
                    break;
                }
                // Raw access units with Apple 4-byte per-frame header
                if ((err = aacEncoder_SetParam(handleAAC, AACENC_TRANSMUX, TT_MP4_RAW)) != AACENC_OK) {
                    printf("Couldn't set AAC-ELD transport: %d\n", err);
                    break;
                }
                if ((err = aacEncEncode(handleAAC, NULL, NULL, NULL, NULL)) != AACENC_OK) {
                    printf("Couldn't initialize AAC-ELD encoder: %d\n", err);
                    break;
                }
                if ((err = aacEncInfo(handleAAC, &aacinf)) != AACENC_OK) {
                    printf("Couldn't get AAC-ELD encoder info: %d\n", err);
                    break;
                }

                printf("AAC-ELD setup complete! frameLength=%d, inputChannels=%d\n",
                       aacinf.frameLength, aacinf.inputChannels);

                current_sample_rate = 48000;
                acc_num_simples = aacinf.frameLength;  // use actual frame size from encoder
                audio_timer_interval = 10;

                printf("AAC-ELD acc_num_simples=%d\n", acc_num_simples);

                audio_slot_queue_configure_with_count(acc_num_simples, AUDIO_SLOT_COUNT_ELD);

                // Core 1 path exists, but FDK-AAC is currently encoded on Core 0.
                aaceld_frame_sequence = 1;
                core1_encoder_active = false;
                printf("AAC-ELD Core 0 encoder active\n");

                cur_codec = 4;  // new codec ID for AAC-ELD

                avdtp_source_open_stream(media_tracker.avdtp_cid, media_tracker.local_seid, media_tracker.remote_seid);
            }
            else
            {
                printf("Config not handled for %s\n", codec_name_for_type(remote_seps[selected_remote_sep_index].sep.capabilities.media_codec.media_codec_type));
            }
            break;

        case AVDTP_SUBEVENT_STREAMING_CAN_SEND_MEDIA_PACKET_NOW:
            a2dp_demo_send_media_packet();
            break;  


        case AVDTP_SUBEVENT_SIGNALING_ACCEPT:
            // TODO check cid
            signal_identifier = avdtp_subevent_signaling_accept_get_signal_identifier(packet);
            if (is_cmd_triggered_localy){
                is_cmd_triggered_localy = 0;
                printf("AVDTP Source command accepted\n");
            }
            
            switch (signal_identifier){
                case AVDTP_SI_OPEN:
                    break;
                case AVDTP_SI_SET_CONFIGURATION:
                    break;
                case  AVDTP_SI_START:
                    printf("Stream started.\n");
                    if (finish_scan_avdtp_codec){
                        a2dp_demo_timer_start(&media_tracker);
                        is_streaming = true;
                        start_led_blink();
                    }
                    break;
                case AVDTP_SI_SUSPEND:
                    printf("Stream paused.\n");
                    // Cancel recovery timer — suspend response arrived
                    suspend_recovery_pending = false;
                    btstack_run_loop_remove_timer(&suspend_recovery_timer);
                    a2dp_demo_timer_pause(&media_tracker);
                    // Auto-restart: ask to resume so AirPods reset their decoder
                    printf("Auto-restarting stream...\n");
                    avdtp_source_start_stream(media_tracker.avdtp_cid, media_tracker.local_seid);
                    break;
                case AVDTP_SI_ABORT:
                case AVDTP_SI_CLOSE:
                    printf("Stream released.\n");
                    a2dp_demo_timer_stop(&media_tracker);
                    break;
                default:
                    break;
            }
            break;

        case AVDTP_SUBEVENT_SIGNALING_REJECT:
            signal_identifier = avdtp_subevent_signaling_reject_get_signal_identifier(packet);
            printf("Rejected %s\n", avdtp_si2str(signal_identifier));
            // If suspend or start was rejected during recovery, restart timer
            if (signal_identifier == AVDTP_SI_SUSPEND || signal_identifier == AVDTP_SI_START) {
                suspend_recovery_pending = false;
                btstack_run_loop_remove_timer(&suspend_recovery_timer);
                if (a2dp_is_connected_flag) {
                    printf("Restarting audio timer after reject\n");
                    a2dp_demo_timer_start(&media_tracker);
                }
            }
            break;
        case AVDTP_SUBEVENT_SIGNALING_GENERAL_REJECT:
            signal_identifier = avdtp_subevent_signaling_general_reject_get_signal_identifier(packet);
            printf("Rejected %s\n", avdtp_si2str(signal_identifier));
            if (signal_identifier == AVDTP_SI_SUSPEND || signal_identifier == AVDTP_SI_START) {
                suspend_recovery_pending = false;
                btstack_run_loop_remove_timer(&suspend_recovery_timer);
                if (a2dp_is_connected_flag) {
                    printf("Restarting audio timer after general reject\n");
                    a2dp_demo_timer_start(&media_tracker);
                }
            }
            break;
        case AVDTP_SUBEVENT_STREAMING_CONNECTION_RELEASED:
            a2dp_demo_timer_stop(&media_tracker);
            printf("Streaming connection released.\n");
            set_led_mode_off();
            is_streaming = false;
            break;
        case AVDTP_SUBEVENT_SIGNALING_CONNECTION_RELEASED:
            a2dp_demo_timer_stop(&media_tracker);
            finish_scan_avdtp_codec = false;
            a2dp_is_connected_flag = false;
            cur_capability = 0;
            set_led_mode_off();
            have_ldac_codec_capabilities = false;
            have_aaceld_codec_capabilities = false;
            finish_setup_aac = false;
            printf("Signaling connection released.\n");
            break;
        default:
            break;
    }
}

void increase_vol_by_key(){
    if (media_tracker.volume > 117){
         media_tracker.volume = 127;
    } else {
                media_tracker.volume += 10;
    }
    printf(" - volume up (via set absolute volume) %d%% (%d)\n",  media_tracker.volume * 100 / 127,  media_tracker.volume);
    avrcp_controller_set_absolute_volume(media_tracker.avrcp_cid, media_tracker.volume);
}


void decrease_vol_by_key(){

    if (media_tracker.volume < 10){
        media_tracker.volume = 0;
    } else {
        media_tracker.volume -= 10;
    }
    printf(" - volume down (via set absolute volume) %d%% (%d)\n",  media_tracker.volume * 100 / 127,  media_tracker.volume);
    avrcp_controller_set_absolute_volume(media_tracker.avrcp_cid, media_tracker.volume);
    
}

#ifdef HAVE_BTSTACK_STDIN

static void show_usage(void){
    bd_addr_t      iut_address;
    gap_local_bd_addr(iut_address);
    printf("\n--- Bluetooth AVDTP SOURCE Test Console %s ---\n", bd_addr_to_str(iut_address));
    printf("c      - create connection to addr %s\n", get_device_addr_string());
    printf("C      - disconnect\n");
    printf("D      - Delete linked list\n");
    printf("z      - select remote endpoint\n");
    printf("g      - set_next_capablity_and_start_stream  %u\n", media_tracker.remote_seid);
    printf("a      - get all capabilities for remote seid %u\n", media_tracker.remote_seid);
    printf("s      - set configuration    for remote seid %u\n", media_tracker.remote_seid);
    printf("f      - get configuration    for remote seid %u\n", media_tracker.remote_seid);
    printf("R      - reconfigure stream   for remote seid %u\n", media_tracker.remote_seid);
    printf("o      - open stream          for remote seid %u\n", media_tracker.remote_seid);
    printf("m      - start stream         for remote seid %u\n", media_tracker.remote_seid);
    printf("A      - abort stream         for remote seid %u\n", media_tracker.remote_seid);
    printf("S      - stop stream          for remote seid %u\n", media_tracker.remote_seid);
    printf("P      - suspend stream       for remote seid %u\n", media_tracker.remote_seid);
    printf("l      - set up ladc          for remote seid %u\n", media_tracker.remote_seid);
    printf("u      - set up sbc           for remote seid %u\n", media_tracker.remote_seid);
    printf("Ctrl-c - exit\n");
    printf("---\n");
}


void set_bt_volume(int16_t val){
    media_tracker.volume = (val*2 + 100) * 127 / 100;
    avrcp_controller_set_absolute_volume(media_tracker.avrcp_cid, media_tracker.volume);
    //printf("(via set usb volume) %d%% (%d)\n",  media_tracker.volume * 100 / 127,  media_tracker.volume);
}

static void stdin_process(char cmd){
    uint8_t status = ERROR_CODE_SUCCESS;
    is_cmd_triggered_localy = 1;
    static bool enter_remote_seid_index = false;

    if (enter_remote_seid_index){
        if ((cmd < '0') || (cmd > '9')) return;
        uint8_t index = cmd - '0';
        if (index >= num_remote_seps){
            printf("Index too high, try again\n");
            return;
        }
        enter_remote_seid_index = false;
        selected_remote_sep_index = index;
        media_tracker.remote_seid = remote_seps[selected_remote_sep_index].sep.seid;
        printf("Selected Remote Stream Endpoint with SEID %u\n",  media_tracker.remote_seid);
        return;
    }

    switch (cmd){
        case 'c':
            printf("Establish AVDTP Source connection to %s\n", get_device_addr_string());
            status = avdtp_source_connect((uint8_t *) get_device_addr(), &media_tracker.avdtp_cid);
            break;
        case 'C':
            printf("Disconnect AVDTP Source\n");
            status = avdtp_source_disconnect(media_tracker.avdtp_cid);
            break;
        case 'D':
            printf("Deleting all link keys\n");
            gap_delete_all_link_keys();
            printf("Finished\n");
            break;
        case 'g':
            printf("set_next_capablity_and_start_stream with seid %d\n", media_tracker.remote_seid);
            set_next_capablity_and_start_stream();
            break;
        case 'a':
            printf("Get all capabilities of stream endpoint with seid %d\n", media_tracker.remote_seid);
            status = avdtp_source_get_all_capabilities(media_tracker.avdtp_cid, media_tracker.remote_seid);
            break;
        case 'f':
            printf("Get configuration of stream endpoint with seid %d\n", media_tracker.remote_seid);
            status = avdtp_source_get_configuration(media_tracker.avdtp_cid, media_tracker.remote_seid);
            break;
        case 'z':
            if (num_remote_seps == 0){
                printf("Remote Stream Endpoints not discovered yet, please discover stream endpoints first\n");
                break;
            }
            dump_remote_sink_endpoints();
            printf("Please enter index of remote stream endpoint:\n");
            enter_remote_seid_index = true;
            break;

        case 's':{
            //status = set_configuration();
            break;
        }
        case 'R':{
            if (num_remote_seps == 0){
                printf("Remote Stream Endpoints not discovered yet, please discover stream endpoints first\n");
                break;
            }
            if (remote_seps[selected_remote_sep_index].have_media_codec_apabilities == false){
                printf("Remote Stream Endpoints Media Codec Capabilities not received yet, please get (all) capabilities for stream endpoint with seid %u first\n",  media_tracker.remote_seid);
                break;
            }
            printf("Reconfigure stream endpoint with seid %d\n", media_tracker.remote_seid);
            avdtp_media_codec_type_t media_codec_type = remote_seps[selected_remote_sep_index].sep.capabilities.media_codec.media_codec_type;
            avdtp_capabilities_t new_configuration;
            new_configuration.media_codec.media_type = AVDTP_AUDIO;

            uint16_t new_sampling_frequency = 44100;
            switch (current_sample_rate){
                case 44100:
                    new_sampling_frequency = 48000;
                    break;
                default:
                    break;
            }

            switch (media_codec_type){
                case AVDTP_CODEC_SBC:
                    avdtp_config_sbc_set_sampling_frequency(media_codec_config_data, new_sampling_frequency);
                    break;
                case AVDTP_CODEC_MPEG_1_2_AUDIO:
                    avdtp_config_mpeg_audio_set_sampling_frequency(media_codec_config_data, new_sampling_frequency);
                    break;
                case AVDTP_CODEC_MPEG_2_4_AAC:
                    avdtp_config_mpeg_aac_set_sampling_frequency(media_codec_config_data, new_sampling_frequency);
                    break;
                case AVDTP_CODEC_ATRAC_FAMILY:
                    avdtp_config_atrac_set_sampling_frequency(media_codec_config_data, new_sampling_frequency);
                    break;
                default:
                    printf("Reconfigure not implemented for %s\n", codec_name_for_type(media_codec_type));
                    return;
            }
            new_configuration.media_codec.media_type = AVDTP_AUDIO;
            new_configuration.media_codec.media_codec_type = media_codec_type;
            new_configuration.media_codec.media_codec_information_len = media_codec_config_len;
            new_configuration.media_codec.media_codec_information = media_codec_config_data;
            status = avdtp_source_reconfigure(media_tracker.avdtp_cid, media_tracker.local_seid, media_tracker.remote_seid, 1 << AVDTP_MEDIA_CODEC, new_configuration);
            break;
        }
        case 'o':
            printf("Establish stream between local %d and remote %d seid\n", media_tracker.local_seid, media_tracker.remote_seid);
            status = avdtp_source_open_stream(media_tracker.avdtp_cid, media_tracker.local_seid, media_tracker.remote_seid);
            break;
        case 'm':
            printf("Start stream between local %d and remote %d seid, \n", media_tracker.local_seid, media_tracker.remote_seid);
            status = avdtp_source_start_stream(media_tracker.avdtp_cid, media_tracker.local_seid);
            break;
        case 'A':
            printf("Abort stream between local %d and remote %d seid\n", media_tracker.local_seid, media_tracker.remote_seid);
            status = avdtp_source_abort_stream(media_tracker.avdtp_cid, media_tracker.local_seid);
            break;
        case 'S':
            printf("Release stream between local %d and remote %d seid\n", media_tracker.local_seid, media_tracker.remote_seid);
            status = avdtp_source_stop_stream(media_tracker.avdtp_cid, media_tracker.local_seid);
            break;
        case 'P':
            printf("Susspend stream between local %d and remote %d seid\n", media_tracker.local_seid, media_tracker.remote_seid);
            status = avdtp_source_suspend(media_tracker.avdtp_cid, media_tracker.local_seid);
            break;
        case 'X':
            printf("Stop streaming\n");
            status = (media_tracker.avdtp_cid, media_tracker.local_seid);
            break;

        case 'l':
            printf("Setting Up  ldac\n");
            status = set_ldac_configuration();
            break;

        case 'v':
            if (media_tracker.volume > 117){
                media_tracker.volume = 127;
            } else {
                media_tracker.volume += 10;
            }
            printf(" - volume up (via set absolute volume) %d%% (%d)\n",  media_tracker.volume * 100 / 127,  media_tracker.volume);
            status = avrcp_controller_set_absolute_volume(media_tracker.avrcp_cid, media_tracker.volume);
            break;
        case 'V':
            if (media_tracker.volume < 10){
                media_tracker.volume = 0;
            } else {
                media_tracker.volume -= 10;
            }
            printf(" - volume down (via set absolute volume) %d%% (%d)\n",  media_tracker.volume * 100 / 127,  media_tracker.volume);
            status = avrcp_controller_set_absolute_volume(media_tracker.avrcp_cid, media_tracker.volume);
            break;

        case 'p':
            a2dp_demo_send_media_packet();
            break;

        case 'u':
            printf("Setup SBC codec\n");
            status = setup_sbc_configuration();
            break;

        case '\n':
        case '\r':
            break;
        default:
            show_usage();
            break;

    }
    if (status != ERROR_CODE_SUCCESS){
        printf("AVDTP Sink cmd \'%c\' failed, status 0x%02x\n", cmd, status);
    }
}
#endif


static int setup_sbc_configuration(){

    uint8_t  sbc_num = 0;
    for (int i = 0; i < num_remote_seps; i++){
        if ( remote_seps[i].sep.capabilities.media_codec.media_codec_type == AVDTP_CODEC_SBC){
            printf("found sbc!!! Remote Stream Endpoints ID is %d\n", i);
            selected_remote_sep_index = i;
            sbc_num = i;
            break;
        }
    }

    // // - SBC (create only once)
    if (stream_endpoint_sbc == NULL) {
        stream_endpoint_sbc = a2dp_source_create_stream_endpoint(AVDTP_AUDIO, AVDTP_CODEC_SBC, (uint8_t *) media_sbc_codec_capabilities, sizeof(media_sbc_codec_capabilities), (uint8_t*) local_stream_endpoint_sbc_media_codec_configuration, sizeof(local_stream_endpoint_sbc_media_codec_configuration));
        btstack_assert(stream_endpoint_sbc != NULL);
        stream_endpoint_sbc->media_codec_configuration_info = local_stream_endpoint_sbc_media_codec_configuration;
        stream_endpoint_sbc->media_codec_configuration_len  = sizeof(local_stream_endpoint_sbc_media_codec_configuration);
    }
    avdtp_source_register_delay_reporting_category(avdtp_local_seid(stream_endpoint_sbc));
    avdtp_set_preferred_sampling_frequency(stream_endpoint_sbc, 48000);
    avdtp_set_preferred_channel_mode(stream_endpoint_sbc, AVDTP_SBC_STEREO);

    // set up local stream_endpoint; need change
    sc.local_stream_endpoint = stream_endpoint_sbc;

    // store local seid
    media_tracker.local_seid  = avdtp_local_seid(sc.local_stream_endpoint);
    media_tracker.remote_seid = remote_seps[sbc_num].sep.seid;


    // choose SBC config params
    const uint8_t * packet = remote_seps[sbc_num].media_codec_event;

    // choose SBC config params
    avdtp_configuration_sbc_t configuration;
    configuration.sampling_frequency = avdtp_choose_sbc_sampling_frequency(sc.local_stream_endpoint, avdtp_subevent_signaling_media_codec_sbc_capability_get_sampling_frequency_bitmap(packet));
    configuration.channel_mode       = avdtp_choose_sbc_channel_mode(sc.local_stream_endpoint, avdtp_subevent_signaling_media_codec_sbc_capability_get_channel_mode_bitmap(packet));
    configuration.block_length       = avdtp_choose_sbc_block_length(sc.local_stream_endpoint, avdtp_subevent_signaling_media_codec_sbc_capability_get_block_length_bitmap(packet));
    configuration.subbands           = avdtp_choose_sbc_subbands(sc.local_stream_endpoint, avdtp_subevent_signaling_media_codec_sbc_capability_get_subbands_bitmap(packet));
    configuration.allocation_method  = avdtp_choose_sbc_allocation_method(sc.local_stream_endpoint, avdtp_subevent_signaling_media_codec_sbc_capability_get_allocation_method_bitmap(packet));
    configuration.max_bitpool_value  = avdtp_choose_sbc_max_bitpool_value(sc.local_stream_endpoint, avdtp_subevent_signaling_media_codec_sbc_capability_get_max_bitpool_value(packet));
    configuration.min_bitpool_value  = avdtp_choose_sbc_min_bitpool_value(sc.local_stream_endpoint, avdtp_subevent_signaling_media_codec_sbc_capability_get_min_bitpool_value(packet));

    // setup SBC configuration
    avdtp_config_sbc_store(media_codec_config_data, &configuration);
    media_codec_config_len = 4;

    avdtp_capabilities_t new_configuration;
    new_configuration.media_codec.media_type = AVDTP_AUDIO;
    new_configuration.media_codec.media_codec_type = remote_seps[selected_remote_sep_index].sep.capabilities.media_codec.media_codec_type ;
    new_configuration.media_codec.media_codec_information_len = media_codec_config_len;
    new_configuration.media_codec.media_codec_information = media_codec_config_data;
    int status = avdtp_source_set_configuration(media_tracker.avdtp_cid, media_tracker.local_seid, media_tracker.remote_seid, 1 << AVDTP_MEDIA_CODEC, new_configuration);
    return status;
}


static int setup_aac_configuration(){

    #ifdef HAVE_AAC_FDK
    // - AAC
    if (stream_endpoint_aac == NULL) {
        stream_endpoint_aac = a2dp_source_create_stream_endpoint(AVDTP_AUDIO, AVDTP_CODEC_MPEG_2_4_AAC, (uint8_t *) media_aac_codec_capabilities, sizeof(media_aac_codec_capabilities), (uint8_t*) local_stream_endpoint_aac_media_codec_configuration, sizeof(local_stream_endpoint_aac_media_codec_configuration));
        btstack_assert(stream_endpoint_aac != NULL);
        stream_endpoint_aac->media_codec_configuration_info = local_stream_endpoint_aac_media_codec_configuration;
        stream_endpoint_aac->media_codec_configuration_len  = sizeof(local_stream_endpoint_aac_media_codec_configuration);
        avdtp_source_register_delay_reporting_category(avdtp_local_seid(stream_endpoint_aac));
    }
    #endif

    uint8_t  aac_num = 0;
    for (int i = 0; i < num_remote_seps; i++){
        if ( remote_seps[i].sep.capabilities.media_codec.media_codec_type == AVDTP_CODEC_MPEG_2_4_AAC){
            printf("found aac!!! Remote Stream Endpoints ID is %d\n", i);
            selected_remote_sep_index = i;
            aac_num = i;
            break;
        }
    }

    // set up local stream_endpoint;
    sc.local_stream_endpoint = stream_endpoint_aac;

    // store local seid
    media_tracker.local_seid  = avdtp_local_seid(sc.local_stream_endpoint);
    media_tracker.remote_seid = remote_seps[aac_num].sep.seid;

    // setup MPEG AAC configuration
    avdtp_configuration_mpeg_aac_t configuration;
    configuration.object_type = 1;
    configuration.sampling_frequency = 48000;
    configuration.channels = 2;

    // if (aac_bit_rate > max_aac_bit_rate){
    //     configuration.bit_rate = max_aac_bit_rate;
    // }else{
    //     configuration.bit_rate = aac_bit_rate;
    // }

    configuration.bit_rate = aac_bit_rate;


    configuration.vbr = 1;
    
    avdtp_config_mpeg_aac_store(media_codec_config_data, &configuration);

    media_codec_config_data[0] = 0x80;
    media_codec_config_len = 6;

    avdtp_capabilities_t new_configuration;
    new_configuration.media_codec.media_type = AVDTP_AUDIO;
    new_configuration.media_codec.media_codec_type =  2,    // Media Codec Type: A2DP_MEDIA_CT_AAC
    new_configuration.media_codec.media_codec_information_len = media_codec_config_len;
    new_configuration.media_codec.media_codec_information = media_codec_config_data;
    int status = avdtp_source_set_configuration(media_tracker.avdtp_cid, media_tracker.local_seid, media_tracker.remote_seid, 1 << AVDTP_MEDIA_CODEC, new_configuration);
    printf("Set AAC Connection Result is %d\n", status);
    finish_setup_aac = true;

    return status;
}



static int set_ldac_configuration(){
    int ladc_num = -1;
    if (num_remote_seps == 0){
        printf("Remote Stream Endpoints not discovered yet, please discover stream endpoints first\n");
        return -1;
    }
    for (int i = 0; i < num_remote_seps; i++){
        if (remote_seps[i].vendor_id == A2DP_CODEC_VENDOR_ID_SONY && remote_seps[i].codec_id == A2DP_SONY_CODEC_LDAC){
            printf("found LDAC!!! Remote Stream Endpoints ID is %d\n", i);
            selected_remote_sep_index = i;
            ladc_num = i;
            break;
        }
    }

    if (ladc_num < 0){
        printf("not found LDAC!!!\n");
        return -1;
    }

    avdtp_media_codec_type_t codec_type = remote_seps[ladc_num].sep.capabilities.media_codec.media_codec_type;
    if (codec_type == AVDTP_CODEC_NON_A2DP) {
        const uint8_t * packet = remote_seps[ladc_num].media_codec_event;
        const uint8_t *media_info = a2dp_subevent_signaling_media_codec_other_capability_get_media_codec_information(packet);
    } else {
        printf("LDAC codec unmatch!!!\n");
        return -1;
    }

    // - LDAC endpoint
    stream_endpoint_ldac = a2dp_source_create_stream_endpoint(AVDTP_AUDIO, AVDTP_CODEC_NON_A2DP, (uint8_t *) media_ldac_codec_capabilities, sizeof(media_ldac_codec_capabilities), (uint8_t*) local_stream_endpoint_ldac_media_codec_configuration, sizeof(local_stream_endpoint_ldac_media_codec_configuration));
    btstack_assert(stream_endpoint_ldac != NULL);
    stream_endpoint_ldac->media_codec_configuration_info = local_stream_endpoint_ldac_media_codec_configuration;
    stream_endpoint_ldac->media_codec_configuration_len  = sizeof(local_stream_endpoint_ldac_media_codec_configuration);
    avdtp_source_register_delay_reporting_category(avdtp_local_seid(stream_endpoint_ldac));

    sc.local_stream_endpoint = stream_endpoint_ldac;

    // store local seid
    media_tracker.local_seid  = avdtp_local_seid(sc.local_stream_endpoint);
    media_tracker.remote_seid = remote_seps[ladc_num].sep.seid;

    // set media configuration
    sc.local_stream_endpoint->remote_configuration_bitmap = store_bit16(sc.local_stream_endpoint->remote_configuration_bitmap, AVDTP_MEDIA_CODEC, 1);
    sc.local_stream_endpoint->remote_configuration.media_codec.media_type = AVDTP_AUDIO;
    sc.local_stream_endpoint->remote_configuration.media_codec.media_codec_type = codec_type;

    media_codec_config_data[0] = 0x2D;
    media_codec_config_data[1] = 0x1;
    media_codec_config_data[2] = 0x0;
    media_codec_config_data[3] = 0x0;  // A2DP_LDAC_VENDOR_ID 0x0000012D

    media_codec_config_data[4] = 0xAA;
    media_codec_config_data[5] = 0x0;  // A2DP_LDAC_CODEC_ID 0x00AA

    media_codec_config_data[6] = 0x10; // A2DP_LDAC_SAMPLING_FREQ_48000

    media_codec_config_data[7] = 0x01; // A2DP_LDAC_CHANNEL_MODE_STEREO

    // Fixed. See 
    // https://android.googlesource.com/platform/packages/modules/Bluetooth
    // system/stack/test/a2dp/media_codec_capabilities_test.cc
    // media_codec_config_data[8] = 0x0;
    // media_codec_config_data[9] = 0x0;

    // not 10 
    media_codec_config_len = 8;

    avdtp_capabilities_t new_configuration;
    new_configuration.media_codec.media_type = AVDTP_AUDIO;
    new_configuration.media_codec.media_codec_type = remote_seps[selected_remote_sep_index].sep.capabilities.media_codec.media_codec_type ;
    new_configuration.media_codec.media_codec_information_len = media_codec_config_len;
    new_configuration.media_codec.media_codec_information = media_codec_config_data;
    int status = avdtp_source_set_configuration(media_tracker.avdtp_cid, media_tracker.local_seid, media_tracker.remote_seid, 1 << AVDTP_MEDIA_CODEC, new_configuration);

    printf("Set LADC Connection Result is %d\n", status);
    return status;
}


static int setup_aaceld_configuration(){
    int aaceld_num = -1;
    if (num_remote_seps == 0){
        printf("Remote Stream Endpoints not discovered yet\n");
        return -1;
    }
    for (int i = 0; i < num_remote_seps; i++){
        if (remote_seps[i].vendor_id == A2DP_CODEC_VENDOR_ID_APPLE && remote_seps[i].codec_id == A2DP_APPLE_CODEC_AAC_ELD){
            printf("found Apple AAC-ELD!!! Remote Stream Endpoints ID is %d\n", i);
            selected_remote_sep_index = i;
            aaceld_num = i;
            break;
        }
    }

    if (aaceld_num < 0){
        printf("not found Apple AAC-ELD!!!\n");
        return -1;
    }

    avdtp_media_codec_type_t codec_type = remote_seps[aaceld_num].sep.capabilities.media_codec.media_codec_type;
    if (codec_type != AVDTP_CODEC_NON_A2DP) {
        printf("AAC-ELD codec type mismatch!!!\n");
        return -1;
    }

    // Create AAC-ELD stream endpoint (only once)
    if (stream_endpoint_aaceld == NULL) {
        stream_endpoint_aaceld = a2dp_source_create_stream_endpoint(
            AVDTP_AUDIO, AVDTP_CODEC_NON_A2DP,
            (uint8_t *) media_aaceld_codec_capabilities, sizeof(media_aaceld_codec_capabilities),
            (uint8_t*) local_stream_endpoint_aaceld_media_codec_configuration,
            sizeof(local_stream_endpoint_aaceld_media_codec_configuration));
        btstack_assert(stream_endpoint_aaceld != NULL);
        stream_endpoint_aaceld->media_codec_configuration_info = local_stream_endpoint_aaceld_media_codec_configuration;
        stream_endpoint_aaceld->media_codec_configuration_len  = sizeof(local_stream_endpoint_aaceld_media_codec_configuration);
        avdtp_source_register_delay_reporting_category(avdtp_local_seid(stream_endpoint_aaceld));
    }

    sc.local_stream_endpoint = stream_endpoint_aaceld;

    media_tracker.local_seid  = avdtp_local_seid(sc.local_stream_endpoint);
    media_tracker.remote_seid = remote_seps[aaceld_num].sep.seid;

    sc.local_stream_endpoint->remote_configuration_bitmap = store_bit16(sc.local_stream_endpoint->remote_configuration_bitmap, AVDTP_MEDIA_CODEC, 1);
    sc.local_stream_endpoint->remote_configuration.media_codec.media_type = AVDTP_AUDIO;
    sc.local_stream_endpoint->remote_configuration.media_codec.media_codec_type = codec_type;

    // Build config: vendor_id(4 LE) + codec_id(2 LE) + params(8)
    media_codec_config_data[0]  = 0x4C;  // vendor_id LE
    media_codec_config_data[1]  = 0x00;
    media_codec_config_data[2]  = 0x00;
    media_codec_config_data[3]  = 0x00;
    media_codec_config_data[4]  = 0x01;  // codec_id LE
    media_codec_config_data[5]  = 0x80;
    media_codec_config_data[6]  = 0x00;  // object_type: AAC-ELD
    media_codec_config_data[7]  = 0x80;
    media_codec_config_data[8]  = 0x00;  // sampling_freq 48kHz (0x008) + channels stereo (0x4)
    media_codec_config_data[9]  = 0x84;
    media_codec_config_data[10] = 0x00;  // reserved
    media_codec_config_data[11] = 0x83;  // VBR=1, bitrate=256000
    media_codec_config_data[12] = 0xE8;
    media_codec_config_data[13] = 0x00;
    media_codec_config_len = 14;

    avdtp_capabilities_t new_configuration;
    new_configuration.media_codec.media_type = AVDTP_AUDIO;
    new_configuration.media_codec.media_codec_type = codec_type;
    new_configuration.media_codec.media_codec_information_len = media_codec_config_len;
    new_configuration.media_codec.media_codec_information = media_codec_config_data;
    int status = avdtp_source_set_configuration(media_tracker.avdtp_cid, media_tracker.local_seid, media_tracker.remote_seid, 1 << AVDTP_MEDIA_CODEC, new_configuration);

    printf("Set Apple AAC-ELD Connection Result is %d\n", status);
    return status;
}


void avdtp_disconnect_and_scan(){
    a2dp_demo_timer_stop(&media_tracker);
    a2dp_source_disconnect(media_tracker.avdtp_cid);
    avrcp_disconnect(media_tracker.avdtp_cid);
    audio_slot_queue_init(); // reset all slots on disconnect
    gap_drop_link_key_for_bd_addr((uint8_t *) get_device_addr());
    uint8_t currect_slot = read_uint8_last_flash();

    printf("currect slot iss %d\n", currect_slot);

    uint8_t new_addr[6] = {0x0};

    if(currect_slot == 0x1){
        write_slot1_mac(new_addr);
    }

    if(currect_slot == 0x2){
        write_slot2_mac(new_addr);
    }
    //gap_delete_all_link_keys();
    gap_start_scanning();
}

void a2dp_source_reconnect(){
    //sleep_ms(500);
    avdtp_source_connect((uint8_t *) get_device_addr(), &media_tracker.avdtp_cid);
    sleep_ms(200);
    //printf(" Create A2DP Source connection to addr %s, cid 0x%02x.\n", bd_addr_to_str(device_addr), media_tracker.a2dp_cid);
}


void avdtp_source_establish_stream(){
    avdtp_source_connect((uint8_t *) get_device_addr(), &media_tracker.avdtp_cid);
}


int set_next_codec(uint8_t num){
    //return 0;
    //return setup_sbc_configuration();
    switch (num){
        case 0: // AAC-ELD > LDAC > AAC > SBC
            if (have_aaceld_codec_capabilities){
                return setup_aaceld_configuration();
            } else if (have_ldac_codec_capabilities){
                return set_ldac_configuration();
            } else {
                // Check if remote actually has AAC before trying
                bool have_aac = false;
                for (int i = 0; i < num_remote_seps; i++){
                    if (remote_seps[i].sep.capabilities.media_codec.media_codec_type == AVDTP_CODEC_MPEG_2_4_AAC){
                        have_aac = true;
                        break;
                    }
                }
                if (have_aac) {
                    return setup_aac_configuration();
                } else {
                    printf("No AAC support, falling back to SBC\n");
                    return setup_sbc_configuration();
                }
            }
        case 1: // sbc
            return setup_sbc_configuration();
        default:
            return 1;
    }
}


void set_next_capablity_and_start_stream(){

    avdtp_source_stop_stream(media_tracker.avdtp_cid, media_tracker.local_seid);

    if (is_streaming){
        return;
    }

    //sleep_ms(200);

    int result = set_next_codec(cur_capability);

    while (result != 0){
        if (result == 1){
            cur_capability = 0;
        }else{
            cur_capability++;
        }
        result = set_next_codec(cur_capability);
        printf("result is %d\n", result);
    }

    cur_capability++;
    finish_scan_avdtp_codec = true;
}


void start_led_blink(){
    set_led_mode_off();
    switch ( cur_codec ) {
        case 1:
            set_led_mode_playing_sbc();
            break;
        case 2:
            set_led_mode_playing_aac();
            break;
        case 4:  // AAC-ELD
            set_led_mode_playing_ldac();
            break;
        case 3:
            set_led_mode_playing_ldac();
            break;
        default:
            break;
    }

}

static bool _bt_sink_volume_changed = false;

// getter returns a pointer to that storage
bool * get_is_bt_sink_volume_changed_ptr(void){
    return &_bt_sink_volume_changed;
}

static void avrcp_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size){
    UNUSED(channel);
    UNUSED(size);
    bd_addr_t event_addr;
    uint16_t local_cid;
    uint8_t  status = ERROR_CODE_SUCCESS;

    if (packet_type != HCI_EVENT_PACKET) return;
    if (hci_event_packet_get_type(packet) != HCI_EVENT_AVRCP_META) return;
    
    switch (packet[2]){
        case AVRCP_SUBEVENT_CONNECTION_ESTABLISHED: 
            local_cid = avrcp_subevent_connection_established_get_avrcp_cid(packet);
            status = avrcp_subevent_connection_established_get_status(packet);
            if (status != ERROR_CODE_SUCCESS){
                printf("AVRCP: Connection failed, local cid 0x%02x, status 0x%02x\n", local_cid, status);
                return;
            }
            media_tracker.avrcp_cid = local_cid;
            avrcp_subevent_connection_established_get_bd_addr(packet, event_addr);

            printf("AVRCP: Channel to %s successfully opened, avrcp_cid 0x%02x\n", bd_addr_to_str(event_addr), media_tracker.avrcp_cid);
             
            avrcp_target_support_event(media_tracker.avrcp_cid, AVRCP_NOTIFICATION_EVENT_PLAYBACK_STATUS_CHANGED);
            avrcp_target_support_event(media_tracker.avrcp_cid, AVRCP_NOTIFICATION_EVENT_TRACK_CHANGED);
            avrcp_target_support_event(media_tracker.avrcp_cid, AVRCP_NOTIFICATION_EVENT_NOW_PLAYING_CONTENT_CHANGED);
            avrcp_target_set_now_playing_info(media_tracker.avrcp_cid, NULL, sizeof(tracks)/sizeof(avrcp_track_t));
            
            printf("Enable Volume Change notification\n");
            avrcp_controller_enable_notification(media_tracker.avrcp_cid, AVRCP_NOTIFICATION_EVENT_VOLUME_CHANGED);
            // Set initial volume to 50% on first connection
            if (media_tracker.volume == 0 || media_tracker.volume == 127) {
                media_tracker.volume = 64;  // ~50% (0-127 range)
                avrcp_controller_set_absolute_volume(media_tracker.avrcp_cid, media_tracker.volume);
            }
            printf("Enable Battery Status Change notification\n");
            avrcp_controller_enable_notification(media_tracker.avrcp_cid, AVRCP_NOTIFICATION_EVENT_BATT_STATUS_CHANGED);
            return;
        
        case AVRCP_SUBEVENT_CONNECTION_RELEASED:
            printf("AVRCP Target: Disconnected, avrcp_cid 0x%02x\n", avrcp_subevent_connection_released_get_avrcp_cid(packet));
            media_tracker.avrcp_cid = 0;
            return;
        default:
            break;
    }

    if (status != ERROR_CODE_SUCCESS){
        printf("Responding to event 0x%02x failed, status 0x%02x\n", packet[2], status);
    }
}


bool is_muted = false;

bool get_bt_mute(){
    return is_muted;
}


static void avrcp_target_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size){
    UNUSED(channel);
    UNUSED(size);
    uint8_t  status = ERROR_CODE_SUCCESS;

    if (packet_type != HCI_EVENT_PACKET) return;
    if (hci_event_packet_get_type(packet) != HCI_EVENT_AVRCP_META) return;

    bool button_pressed;
    char const * button_state;
    avrcp_operation_id_t operation_id;

    switch (packet[2]){
        case AVRCP_SUBEVENT_PLAY_STATUS_QUERY:
            status = avrcp_target_play_status(media_tracker.avrcp_cid, play_info.song_length_ms, play_info.song_position_ms, play_info.status);            
            break;
        // case AVRCP_SUBEVENT_NOW_PLAYING_INFO_QUERY:
        //     status = avrcp_target_now_playing_info(avrcp_cid);
        //     break;
        case AVRCP_SUBEVENT_OPERATION:
            operation_id = avrcp_subevent_operation_get_operation_id(packet);
            button_pressed = avrcp_subevent_operation_get_button_pressed(packet) > 0;
            button_state = button_pressed ? "PRESS" : "RELEASE";

            printf("AVRCP Target: operation %s (%s)\n", avrcp_operation2str(operation_id), button_state);

            if (!button_pressed){
                break;
            }
            switch (operation_id) {
                case AVRCP_OPERATION_ID_PLAY:
                    is_muted = false;
                    _bt_sink_volume_changed = true;
                    avrcp_controller_set_absolute_volume(media_tracker.avrcp_cid, media_tracker.volume);
                    //a2dp_demo_timer_start(&media_tracker);
                    break;
                case AVRCP_OPERATION_ID_PAUSE:
                    //a2dp_demo_timer_pause(&media_tracker);
                    _bt_sink_volume_changed = true;
                    is_muted = true;
                    avrcp_controller_set_absolute_volume(media_tracker.avrcp_cid, 0);
                    break;
                case AVRCP_OPERATION_ID_STOP:
                    status = avdtp_source_stop_stream(media_tracker.avdtp_cid, media_tracker.local_seid);
                    break;
                default:
                    break;
            }
            break;
        default:
            break;
    }

    if (status != ERROR_CODE_SUCCESS){
        printf("Responding to event 0x%02x failed, status 0x%02x\n", packet[2], status);
    }
}

uint8_t get_bt_volume(){
    return media_tracker.volume;
}

static void avrcp_controller_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size){
    UNUSED(channel);
    UNUSED(size);
    
    if (packet_type != HCI_EVENT_PACKET) return;
    if (hci_event_packet_get_type(packet) != HCI_EVENT_AVRCP_META) return;
    if (!media_tracker.avrcp_cid) return;
    
    switch (packet[2]){
        case AVRCP_SUBEVENT_NOTIFICATION_VOLUME_CHANGED:
            media_tracker.volume = avrcp_subevent_notification_volume_changed_get_absolute_volume(packet);
            
            _bt_sink_volume_changed = true;
            printf("AVRCP Controller: Notification Absolute Volume %d %%\n", media_tracker.volume * 100 / 127);\

            break;
        case AVRCP_SUBEVENT_NOTIFICATION_EVENT_BATT_STATUS_CHANGED:
            // see avrcp_battery_status_t
            printf("AVRCP Controller: Notification Battery Status 0x%02x\n", avrcp_subevent_notification_event_batt_status_changed_get_battery_status(packet));
            break;
        case AVRCP_SUBEVENT_NOTIFICATION_STATE:
            printf("AVRCP Controller: Notification %s - %s\n", 
                avrcp_event2str(avrcp_subevent_notification_state_get_event_id(packet)), 
                avrcp_subevent_notification_state_get_enabled(packet) != 0 ? "enabled" : "disabled");
            break;
        default:
            break;
    }  
}

int btstack_main(int argc, const char * argv[]){

    l2cap_init();
    bt_hci_init();

    // Initialize AVDTP Sink
    avdtp_source_init();
    avdtp_source_register_packet_handler(&packet_handler);

    // Initialize AVRCP Service
    avrcp_init();
    avrcp_register_packet_handler(&avrcp_packet_handler);
    // Initialize AVRCP Target
    avrcp_target_init();
    avrcp_target_register_packet_handler(&avrcp_target_packet_handler);

    // Initialize AVRCP Controller
    avrcp_controller_init();
    avrcp_controller_register_packet_handler(&avrcp_controller_packet_handler);


    // Initialize SDP
    sdp_init();
    memset(sdp_avdtp_source_service_buffer, 0, sizeof(sdp_avdtp_source_service_buffer));
    a2dp_source_create_sdp_record(sdp_avdtp_source_service_buffer, 0x10002, AVDTP_SOURCE_FEATURE_MASK_PLAYER, NULL, NULL);
    sdp_register_service(sdp_avdtp_source_service_buffer);



    // Create AVRCP Target service record and register it with SDP. We receive Category 1 commands from the headphone, e.g. play/pause
    memset(sdp_avrcp_target_service_buffer, 0, sizeof(sdp_avrcp_target_service_buffer));
    uint16_t supported_features = AVRCP_FEATURE_MASK_CATEGORY_PLAYER_OR_RECORDER;
#ifdef AVRCP_BROWSING_ENABLED
    supported_features |= AVRCP_FEATURE_MASK_BROWSING;
#endif
    avrcp_target_create_sdp_record(sdp_avrcp_target_service_buffer, 0x10002, supported_features, NULL, NULL);
    sdp_register_service(sdp_avrcp_target_service_buffer);

    // Create AVRCP Controller service record and register it with SDP. We send Category 2 commands to the headphone, e.g. volume up/down
    memset(sdp_avrcp_controller_service_buffer, 0, sizeof(sdp_avrcp_controller_service_buffer));
    uint16_t controller_supported_features = AVRCP_FEATURE_MASK_CATEGORY_MONITOR_OR_AMPLIFIER;
    avrcp_controller_create_sdp_record(sdp_avrcp_controller_service_buffer, 0x10003, controller_supported_features, NULL, NULL);
    sdp_register_service(sdp_avrcp_controller_service_buffer);


#ifdef HAVE_BTSTACK_STDIN
    btstack_stdin_setup(stdin_process);
#endif

    // turn on!
    hci_power_control(HCI_POWER_ON);

    return 0;

}
