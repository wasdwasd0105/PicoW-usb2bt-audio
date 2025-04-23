//
// Created by  on 8/10/23.
//

#include <stdint.h>
#include "btstack.h"


#ifndef PICOW_USB_BT_AUDIO_BTSTACK_HCI_H
#define PICOW_USB_BT_AUDIO_BTSTACK_HCI_H

static void hci_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size);
void bt_hci_init(void);

const char * get_device_addr_string();
bd_addr_t * get_device_addr();



#endif //PICOW_USB_BT_AUDIO_BTSTACK_HCI_H
