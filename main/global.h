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

#ifndef GLOBAL_H
#define GLOBAL_H

#define ESP_INTR_FLAG_DEFAULT 0

#define WIFI_CONNECT_MAXIMUM_RETRY 3
#define WIFI_SSID_MAX_LEN 32 // This max len are not configs. Defined by esp-idf
#define WIFI_PASSWORD_MAX_LEN 64 // This max len are not configs. Defined by esp-idf

#define LED_COUNT 1
#define LED_1_PIN 4

#define MQTT_BROKER_URL "mqtt://10.128.1.5"
#define MQTT_BODY_DETECTION_PUBLISH_TOPIC "/status/poopal/bodydet"

#define MQTT_CONFIG_SUBSCRIBE_TOPIC "/config/poopal/#"
#define MQTT_CONFIG_BODY_DETECTION_ENABLED_TOPIC "/config/poopal/bodydet/enabled"
#define MQTT_CONFIG_BODY_DETECTION_DELAY_TOPIC "/config/poopal/bodydet/delay"

#define SMOOTH_AVERAGE_WEIGHT 0.5f

#define DEVICE_STATUS_COLLECT_INTERVAL_MS 500

#define BODY_DETECTION_PIN 21
#define BODY_DETECTION_DEFAULT_ENABLED true
#define BODY_DETECTION_DEFAULT_GRACE_PERIOD_SECONDS 5
#define BODY_DETECTION_LOW_ACTIVE true // Inverted?

#define DEVICE_STATUS_PUBLISH_INTERVAL_MS 2000

#define LOG_TAG_WIFI "app.wifi"
#define LOG_TAG_APP "app"
#define LOG_TAG_MQTT "app.mqtt"
#define LOG_TAG_DEVICE_CONTROL "app.devctrl"
#define LOG_TAG_BODY_DETECTION "app.bodydet"
#define LOG_TAG_LED "app.led"
#define LOG_TAG_TIMEMAN "app.timeman"
#define LOG_TAG_AZIOT "app.aziot"


#define UNUSED(x) (void)(x)
#define fldsiz(name, field) \
    (sizeof(((struct name *)0)->field))
#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

inline static float smooth_average(float current, float previous)
{
    return SMOOTH_AVERAGE_WEIGHT * current + (1 - SMOOTH_AVERAGE_WEIGHT) * previous;
}

#include "creddef.h"

#ifndef WIFI_SSID
#error "Define `WIFI_SSID' in build environment variable"
#endif // !WIFI_SSID

#ifndef WIFI_PASS
#error "Define `WIFI_PASS' in build environment variable"
#endif // !WIFI_PASS


#endif // GLOBAL_H
