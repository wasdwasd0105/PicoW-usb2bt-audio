//
// Created by Sean on 8/10/23.
//

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>

#include "btstack_hci.h"
#include "btstack_avdtp_source.h"
#include "../pico_w_led.h"


#define A2DP_SOURCE_DEMO_INQUIRY_DURATION_1280MS 12

static btstack_packet_callback_registration_t hci_event_callback_registration;

static char device_addr_string[] = "00:00:00:00:00:00";
static bd_addr_t device_addr;
static bd_addr_t device_addr_list[2];

static bool scan_active = true;


const char * get_device_addr_string(){
    return device_addr_string;
}

bd_addr_t * get_device_addr(){
    return &device_addr;
}

bd_addr_t * get_device_addr_from_list(uint8_t i){
    return &device_addr_list[i];
}

void gap_start_scanning(void){
    printf("Start scanning...\n");
    gap_inquiry_start(A2DP_SOURCE_DEMO_INQUIRY_DURATION_1280MS);
    scan_active = true;
    set_led_mode_off();
    set_led_mode_pairing();
}


static void hci_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size){
    UNUSED(channel);
    UNUSED(size);
    if (packet_type != HCI_EVENT_PACKET) return;
    uint8_t status;
    UNUSED(status);

    bd_addr_t address;
    uint32_t cod;

    // Service Class: Rendering | Audio, Major Device Class: Audio
    const uint32_t bluetooth_speaker_cod = 0x200000 | 0x040000 | 0x000400;

    switch (hci_event_packet_get_type(packet)){
        case  BTSTACK_EVENT_STATE:
            if (btstack_event_state_get_state(packet) != HCI_STATE_WORKING) return;
            if ( strcmp(device_addr_string , "00:00:00:00:00:00") == 0 ){
                gap_start_scanning();
            }
            break;

        case HCI_EVENT_PIN_CODE_REQUEST:
            printf("Pin code request - using '0000'\n");
            hci_event_pin_code_request_get_bd_addr(packet, address);
            gap_pin_code_response(address, "0000");
            break;
        case GAP_EVENT_INQUIRY_RESULT:
            gap_event_inquiry_result_get_bd_addr(packet, address);
            // print info
            printf("Device found: %s ",  bd_addr_to_str(address));
            cod = gap_event_inquiry_result_get_class_of_device(packet);
            printf("with COD: %06" PRIx32, cod);
            if (gap_event_inquiry_result_get_rssi_available(packet)){
                printf(", rssi %d dBm", (int8_t) gap_event_inquiry_result_get_rssi(packet));
            }
            if (gap_event_inquiry_result_get_name_available(packet)){
                char name_buffer[240];
                int name_len = gap_event_inquiry_result_get_name_len(packet);
                memcpy(name_buffer, gap_event_inquiry_result_get_name(packet), name_len);
                name_buffer[name_len] = 0;
                printf(", name '%s'", name_buffer);
            }
            printf("\n");
            if ((cod & bluetooth_speaker_cod) == bluetooth_speaker_cod){
                memcpy(device_addr, address, 6);
                printf("Bluetooth speaker detected, trying to connect to %s...\n", bd_addr_to_str(device_addr));
                scan_active = false;
                gap_inquiry_stop();

                uint8_t currect_slot = read_uint8_last_flash();

                if (currect_slot == 0x1){
                    write_slot1_mac(device_addr);
                }
    
                if (currect_slot == 0x2){
                    write_slot2_mac(device_addr);
                }
                avdtp_source_establish_stream();
            }
            break;
        case GAP_EVENT_INQUIRY_COMPLETE:
            if (scan_active){
                printf("No Bluetooth speakers found, scanning again...\n");
                gap_inquiry_start(A2DP_SOURCE_DEMO_INQUIRY_DURATION_1280MS);
            }
            break;
        default:
            break;
    }
}


void get_link_keys(void){
    bd_addr_t  addr;
    link_key_t link_key;
    link_key_type_t type;
    btstack_link_key_iterator_t it;
    const char * addr_str;

    int ok = gap_link_key_iterator_init(&it);
    if (!ok) {
        printf("Link key iterator not implemented\n");
        return;
    }
    printf("Stored First link key: \n");

    if (gap_link_key_iterator_get_next(&it, addr, link_key, &type)){
        addr_str = bd_addr_to_str(addr);
        printf("%s - type %u, key: ", addr_str, (int) type);
        printf_hexdump(link_key, 16);
        strncpy(device_addr_string, addr_str, sizeof(device_addr_string) - 1);
        sscanf_bd_addr(device_addr_string, device_addr_list[0]);
    }

    //sscanf_bd_addr(device_addr_string, device_addr);

    if (gap_link_key_iterator_get_next(&it, addr, link_key, &type)){
        addr_str = bd_addr_to_str(addr);
        printf("%s - type %u, key: ", addr_str, (int) type);
        printf_hexdump(link_key, 16);
        strncpy(device_addr_string, addr_str, sizeof(device_addr_string) - 1);
        sscanf_bd_addr(device_addr_string, device_addr_list[1]);
    }

    printf(".\n");
    gap_link_key_iterator_done(&it);

    uint8_t currect_slot = read_uint8_last_flash();

    printf("currect slot iss %d\n", currect_slot);

    if(currect_slot == 0x1){
        set_led_mode_double_blink();
        bd_addr_t  addr_flash_slot1;
        read_slot1_mac(addr_flash_slot1);
        printf("cur slot1 flash addr is %s\n ", bd_addr_to_str(addr_flash_slot1));
        memcpy(device_addr, addr_flash_slot1, sizeof(bd_addr_t)); 
    }

    if(currect_slot == 0x2){
        //printf("currect slot is 2\n");
        set_led_mode_triple_blink();
        bd_addr_t  addr_flash_slot2;
        read_slot2_mac(addr_flash_slot2);
        printf("cur slot2 flash addr is %s\n ", bd_addr_to_str(addr_flash_slot2));
        memcpy(device_addr, addr_flash_slot2, sizeof(bd_addr_t)); 
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


void bt_hci_init(void){

    // inform about BTstack state
    hci_event_callback_registration.callback = &packet_handler;
    hci_add_event_handler(&hci_event_callback_registration);

    // Request role change on reconnecting headset to always use them in slave mode
    hci_set_master_slave_policy(0);
    // enabled EIR
    hci_set_inquiry_mode(INQUIRY_MODE_RSSI_AND_EIR);

    // Set local name with a template Bluetooth address, that will be automatically
    // replaced with a actual address once it is available, i.e. when BTstack boots
    // up and starts talking to a Bluetooth module.
    gap_set_local_name("Pico USB Audio");
    gap_discoverable_control(0);
    gap_set_class_of_device(0x200408);

    /* Register for HCI events */
    hci_event_callback_registration.callback = &hci_packet_handler;
    hci_add_event_handler(&hci_event_callback_registration);

    get_link_keys();

}