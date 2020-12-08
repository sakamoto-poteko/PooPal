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

#ifndef DEVICECONTROLFLOW_H
#define DEVICECONTROLFLOW_H

#include "global.h"

void init_device_control_logic();
void start_device_control_logic();

typedef enum device_control_event_type_t {
    DEVICE_CONTROL_EVENT_BODY_DETECTION_ENABLED,
    DEVICE_CONTROL_EVENT_BODY_DETECTION_DISABLED,
    DEVICE_CONTROL_EVENT_BODY_DETECTION_TRIGGERED,
    DEVICE_CONTROL_EVENT_BODY_DETECTION_DELAY_CHANGED,

    DEVICE_CONTROL_EVENT_WIFI_DISCONNECTED,
    DEVICE_CONTROL_EVENT_WIFI_ASSOCIATED,
    DEVICE_CONTROL_EVENT_WIFI_CONNECTED,
    DEVICE_CONTROL_EVENT_WIFI_CONNECTING,
    DEVICE_CONTROL_EVENT_WIFI_FAILED,

    DEVICE_CONTROL_EVENT_MQTT_CONNECTED,
    DEVICE_CONTROL_EVENT_MQTT_DISCONNECTED
} device_control_event_type;

typedef struct device_control_event_t {
    device_control_event_type event_type;
    union {
        int body_detected;
        unsigned int body_detection_delay_seconds;
    };
} device_control_event;

void device_control_send_event(device_control_event *event);

#endif // DEVICECONTROLFLOW_H
