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
#include "timeman.h"

#define NVS_KEY_CONFIG_BODY_DETECTION_ENABLED "bodydet"
#define NVS_KEY_CONFIG_BODY_DETECTION_GRACE_PERIOD "bodydetdelay"

typedef struct device_control_config_t
{
    bool body_detection_enabled;
    uint body_detection_delay_seconds;
} device_control_config;

static device_control_config _config;

static bool _body_detected = false;
static xQueueHandle _device_control_event_queue;
static TimerHandle_t _body_detection_grace_period_timer;
static nvs_handle _nvs_config_handle;
static TickType_t _body_detection_delay_grace_period_ticks;

inline static void write_nvs_config_body_detection_enabled(bool enabled)
{
    nvs_set_u8(_nvs_config_handle, NVS_KEY_CONFIG_BODY_DETECTION_ENABLED, enabled);
}

inline static void write_nvs_config_body_detection_grace_period(uint delay)
{
    nvs_set_u32(_nvs_config_handle, NVS_KEY_CONFIG_BODY_DETECTION_GRACE_PERIOD, delay);
}

void body_detection_grace_period_timeout(TimerHandle_t xTimer)
{
    UNUSED(xTimer);
    _body_detected = false;
    ESP_LOGI(LOG_TAG_DEVICE_CONTROL, "body detection delay timed out");

    // TODO: add it to mqtt send queue
}

static void device_control_task(void *arg)
{
    UNUSED(arg);
    for (;;)
    {
        heap_caps_check_integrity_all(true);
        device_control_event event = {};
        if (xQueueReceive(_device_control_event_queue, &event, portMAX_DELAY))
        {
            switch (event.event_type)
            {
                // Body detection

                // enable body detection
                case DEVICE_CONTROL_EVENT_BODY_DETECTION_ENABLED:
                    _config.body_detection_enabled = true;
                    write_nvs_config_body_detection_enabled(true);
                    break;
                
                // disable body detection
                case DEVICE_CONTROL_EVENT_BODY_DETECTION_DISABLED:
                    _config.body_detection_enabled = false;
                    write_nvs_config_body_detection_enabled(false);
                    break;

                // set body detection grace
                case DEVICE_CONTROL_EVENT_BODY_DETECTION_DELAY_CHANGED:
                    _config.body_detection_delay_seconds = event.body_detection_delay_seconds;
                    write_nvs_config_body_detection_grace_period(event.body_detection_delay_seconds);
                    _body_detection_delay_grace_period_ticks = _config.body_detection_delay_seconds * 1000 / portTICK_PERIOD_MS;
                    break;

                // body detection triggered
                case DEVICE_CONTROL_EVENT_BODY_DETECTION_TRIGGERED:
                {
                    int detected = get_body_detected();
                    if (_config.body_detection_enabled)
                    {
                        if (detected)
                        {
                            // set body detected to true
                            _body_detected = true;

                            // stop the grace period timer, since
                            // 1. if it's in grace period now: we're merging two detections
                            // 2. if it's not in grace period: the timer isn't running anyway
                            // TODO:

                            // if this is a new detection, i.e. not in grace period
                            // store the time now as start time
                            // TODO:
                            // else, i.e. detected in grace period
                            // the start time is not changed
                            // TODO:
                        }
                        else
                        {
                            // body has gone. start the grace period timer
                            xTimerChangePeriod(_body_detection_grace_period_timer, _body_detection_delay_grace_period_ticks, portMAX_DELAY);
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
                    timeman_start();
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
    ESP_ERROR_CHECK(nvs_open("config", NVS_READWRITE, &_nvs_config_handle));

    uint8_t u8bodydet_enabled;

    err = nvs_get_u8(_nvs_config_handle, NVS_KEY_CONFIG_BODY_DETECTION_ENABLED, &u8bodydet_enabled);
    _config.body_detection_enabled = u8bodydet_enabled;
    if (err != ESP_OK)
    {
        ESP_LOGE(LOG_TAG_DEVICE_CONTROL, "failed to read NVS config fan body detection enabled: %s", esp_err_to_name(err));
        _config.body_detection_enabled = BODY_DETECTION_DEFAULT_ENABLED;
        if (err == ESP_ERR_NVS_NOT_FOUND)
        {
            write_nvs_config_body_detection_enabled(BODY_DETECTION_DEFAULT_ENABLED);
        }
    }
    else
    {
        ESP_LOGI(LOG_TAG_DEVICE_CONTROL, "config - body detection: %s", u8bodydet_enabled ? "enabled" : "disabled");
    }

    err = nvs_get_u32(_nvs_config_handle, NVS_KEY_CONFIG_BODY_DETECTION_GRACE_PERIOD, &_config.body_detection_delay_seconds);
    if (err != ESP_OK)
    {
        ESP_LOGE(LOG_TAG_DEVICE_CONTROL, "failed to read NVS config body detection delay: %s", esp_err_to_name(err));
        _config.body_detection_delay_seconds = BODY_DETECTION_DEFAULT_GRACE_PERIOD_SECONDS;
        if (err == ESP_ERR_NVS_NOT_FOUND)
        {
            write_nvs_config_body_detection_grace_period(BODY_DETECTION_DEFAULT_GRACE_PERIOD_SECONDS);
        }
    }
    else
    {
        ESP_LOGI(LOG_TAG_DEVICE_CONTROL, "config - body detection delay: %d", _config.body_detection_delay_seconds);
    }
}

void init_device_control_logic()
{
    read_config_from_nvs();

    _body_detection_delay_grace_period_ticks = _config.body_detection_delay_seconds * 1000 / portTICK_PERIOD_MS;
    _body_detection_grace_period_timer = xTimerCreate("body_detection_timer",
                                               _body_detection_delay_grace_period_ticks,
                                               pdFALSE,
                                               (void *)0,
                                               body_detection_grace_period_timeout);

    _device_control_event_queue = xQueueCreate(20, sizeof(device_control_event));
}

void start_device_control_logic()
{
    xTaskCreate(device_control_task, "device_control_task", 4096, NULL, 0, NULL);
}

void device_control_send_event(device_control_event *event)
{
    xQueueSend(_device_control_event_queue, event, portMAX_DELAY);
}
