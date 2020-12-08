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

#include <stdatomic.h>

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "freertos/timers.h"

#include "esp_log.h"
#include "nvs.h"

#include "global.h"
#include "devicecontrollogic.h"
#include "bodydetection.h"
#include "wifi.h"
#include "led.h"

#define NVS_KEY_CONFIG_BODY_DETECTION_ENABLED "bodydet"
#define NVS_KEY_CONFIG_BODY_DETECTION_DELAY "bodydetdelay"

typedef struct device_control_config_t {
    bool body_detection_enabled;
    uint body_detection_delay_seconds;
} device_control_config;

static device_control_config config;

static bool body_detected = false;
xQueueHandle device_control_event_queue;
TimerHandle_t body_detection_delay_timer;
nvs_handle nvs_config_handle;

static TickType_t body_detection_delay_timer_delay;


inline static void write_nvs_config_body_detection_enabled(bool enabled)
{
    nvs_set_u8(nvs_config_handle, NVS_KEY_CONFIG_BODY_DETECTION_ENABLED, enabled);
}

inline static void write_nvs_config_body_detection_delay(uint delay)
{
    nvs_set_u32(nvs_config_handle, NVS_KEY_CONFIG_BODY_DETECTION_DELAY, delay);
}

void body_detection_delay_timeout(TimerHandle_t xTimer)
{
    UNUSED(xTimer);
    body_detected = false;
    ESP_LOGI(LOG_TAG_DEVICE_CONTROL, "body detection delay timed out");
}

static void device_control_task(void* arg)
{
    UNUSED(arg);
    for(;;) {
        heap_caps_check_integrity_all(true);
        device_control_event event = {};
        if(xQueueReceive(device_control_event_queue, &event, portMAX_DELAY)) {
            switch (event.event_type) {


                // Body detection
            case DEVICE_CONTROL_EVENT_BODY_DETECTION_ENABLED:
                config.body_detection_enabled = true;
                write_nvs_config_body_detection_enabled(true);
                break;
            case DEVICE_CONTROL_EVENT_BODY_DETECTION_DISABLED:
                config.body_detection_enabled = false;
                write_nvs_config_body_detection_enabled(false);
                break;
            case DEVICE_CONTROL_EVENT_BODY_DETECTION_DELAY_CHANGED:
                config.body_detection_delay_seconds = event.body_detection_delay_seconds;
                write_nvs_config_body_detection_delay(event.body_detection_delay_seconds);
                body_detection_delay_timer_delay = config.body_detection_delay_seconds * 1000 / portTICK_PERIOD_MS;
                break;
            case DEVICE_CONTROL_EVENT_BODY_DETECTION_TRIGGERED:
            {
                int detected = get_body_detected();
                if (config.body_detection_enabled) {
                    if (detected) {
                        body_detected = true;
                    } else {
                        // body has gone. start timer
                        xTimerChangePeriod(body_detection_delay_timer, body_detection_delay_timer_delay, portMAX_DELAY);
                    }
                }
                break;
            }
                // WiFi
                // Flash LED?
            case DEVICE_CONTROL_EVENT_WIFI_DISCONNECTED:
                set_led_fade_in_out(LED_1, 1000, 1000);
                break;
            case DEVICE_CONTROL_EVENT_WIFI_ASSOCIATED:
                set_led_flash(LED_1, 100);
                break;
            case DEVICE_CONTROL_EVENT_WIFI_CONNECTED:
                set_led_on(LED_1);
                break;
            case DEVICE_CONTROL_EVENT_WIFI_CONNECTING:
                set_led_flash(LED_1, 300);
                break;
            case DEVICE_CONTROL_EVENT_WIFI_FAILED:
                break;
            default:
                break;
            }
        }
    }
}

static void read_config_from_nvs()
{
    esp_err_t err;
    ESP_ERROR_CHECK(nvs_open("config", NVS_READWRITE, &nvs_config_handle));

    uint8_t u8bodydet_enabled;

    err = nvs_get_u8(nvs_config_handle, NVS_KEY_CONFIG_BODY_DETECTION_ENABLED, &u8bodydet_enabled);
    config.body_detection_enabled = u8bodydet_enabled;
    if (err != ESP_OK) {
        ESP_LOGE(LOG_TAG_DEVICE_CONTROL, "failed to read NVS config fan body detection enabled: %s", esp_err_to_name(err));
        config.body_detection_enabled = BODY_DETECTION_DEFAULT_ENABLED;
        if (err == ESP_ERR_NVS_NOT_FOUND) {
            write_nvs_config_body_detection_enabled(BODY_DETECTION_DEFAULT_ENABLED);
        }
    } else {
        ESP_LOGI(LOG_TAG_DEVICE_CONTROL, "config - body detection: %s", u8bodydet_enabled ? "enabled" : "disabled");
    }

    err = nvs_get_u32(nvs_config_handle, NVS_KEY_CONFIG_BODY_DETECTION_DELAY, &config.body_detection_delay_seconds);
    if (err != ESP_OK) {
        ESP_LOGE(LOG_TAG_DEVICE_CONTROL, "failed to read NVS config body detection delay: %s", esp_err_to_name(err));
        config.body_detection_delay_seconds = BODY_DETECTION_DEFAULT_DELAY_SECONDS;
        if (err == ESP_ERR_NVS_NOT_FOUND) {
            write_nvs_config_body_detection_delay(BODY_DETECTION_DEFAULT_DELAY_SECONDS);
        }
    } else {
        ESP_LOGI(LOG_TAG_DEVICE_CONTROL, "config - body detection delay: %d", config.body_detection_delay_seconds);
    }
}

void init_device_control_logic()
{
    read_config_from_nvs();

    body_detection_delay_timer_delay = config.body_detection_delay_seconds * 1000 / portTICK_PERIOD_MS;
    body_detection_delay_timer = xTimerCreate("body_detection_timer",
                                                 body_detection_delay_timer_delay,
                                                 pdFALSE,
                                                 ( void * ) 0,
                                                 body_detection_delay_timeout
                                                 );

    device_control_event_queue = xQueueCreate(20, sizeof (device_control_event));
}

void start_device_control_logic()
{
    xTaskCreate(device_control_task, "device_control_task", 4096, NULL, 0, NULL);
}

void device_control_send_event(device_control_event *event)
{
    xQueueSend(device_control_event_queue, event, portMAX_DELAY);
}
