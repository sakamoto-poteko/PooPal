// <BEGIN LICENSE>
/*************************************************************************
 * Copyright 2020 Afa Cheng <afa@afa.moe>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the 
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 *  in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL 
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN 
 * THE SOFTWARE.
 **************************************************************************/
// <END LICENSE>

#ifndef DATALINK_H
#define DATALINK_H

#include <stdint.h>

#include "mqtt_client.h"

extern esp_mqtt_client_handle_t __mqtt_client;

void init_datalink(bool mqtt_enabled, bool azure_iot_enabled);
void start_datalink();

void publish_device_status();

typedef enum data_link_event_type_t {
    DATA_LINK_EVENT_BODY_DETECTION
} data_link_event_type;

typedef struct data_link_body_detection_event_t {
    uint64_t start_epoch_second;
    uint64_t elapsed_second;
} data_link_body_detection_event;

typedef struct data_link_event_t {
    data_link_event_type event_type;
    union {
        data_link_body_detection_event body_detection_event;
    };
} data_link_event;

void datalink_send_event(data_link_event *event);

#endif // DATALINK_H
