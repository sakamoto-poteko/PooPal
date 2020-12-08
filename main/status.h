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

#ifndef STATUS_H
#define STATUS_H

typedef enum WifiStatus_t {
    // When modify this enum, change the corresponding BLE doc/behavior as well
    WIFI_STATUS_DISCONNECTED = 0,
    WIFI_STATUS_CONNECTED = 1,
    WIFI_STATUS_CONNECTING = 2,
} WifiStatus;

typedef enum DatalinkStatus_t {
    DATALINK_STATUS_DISCONNECTED,
    DATALINK_STATUS_CONNECTED,
} DatalinkStatus;

typedef struct Status_t {
    float front_fan_dutycycle;
    float rear_fan_dutycycle;
    float front_fan_rpm;
    float rear_fan_rpm;
    int body_detected;
    int fan_enabled;
    WifiStatus wifi_status;
    DatalinkStatus datalink_status;
} DeviceStatus;

extern DeviceStatus __device_status;

void collect_device_status_task(void *pvParameters);
void publish_device_status_task(void *pvParameters);

#endif // STATUS_H
