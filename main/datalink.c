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

#include <stdlib.h>

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include "esp_log.h"

#include "global.h"

#include "datalink.h"
#include "devicecontrollogic.h"
#include "status.h"

typedef struct datalink_config_t {
 bool enable_mqtt;
 bool enable_azure_iot;
 esp_mqtt_client_handle_t mqtt_client;
 xQueueHandle datalink_event_queue;
} datalink_config;

static datalink_config _config;


static void init_mqtt(void);
static void start_mqtt(void);
static void subscribe_mqtt_topics(esp_mqtt_client_handle_t client);
static void process_downlink_data(const char* topic, int topic_len, const char* data, int data_len);

static esp_err_t mqtt_event_handler(esp_mqtt_event_handle_t event)
{
    esp_mqtt_client_handle_t client = event->client;
    switch (event->event_id) {
    case MQTT_EVENT_CONNECTED:
        subscribe_mqtt_topics(client);
        __device_status.datalink_status = DATALINK_STATUS_CONNECTED;
        {
            device_control_event e;
            e.event_type = DEVICE_CONTROL_EVENT_MQTT_CONNECTED;
            device_control_send_event(&e);
        }
        break;
    case MQTT_EVENT_DISCONNECTED:
        __device_status.datalink_status = DATALINK_STATUS_DISCONNECTED;
        {
            device_control_event e;
            e.event_type = DEVICE_CONTROL_EVENT_MQTT_DISCONNECTED;
            device_control_send_event(&e);
        }
        ESP_LOGI(LOG_TAG_MQTT, "MQTT_EVENT_DISCONNECTED");
        break;
    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(LOG_TAG_MQTT, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(LOG_TAG_MQTT, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        break;
    case MQTT_EVENT_DATA:
        process_downlink_data(event->topic, event->topic_len, event->data, event->data_len);
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI(LOG_TAG_MQTT, "MQTT_EVENT_ERROR");
        break;
    default:
        ESP_LOGI(LOG_TAG_MQTT, "Other event id: %d", event->event_id);
        break;
    }
    return ESP_OK;
}

void init_mqtt(void)
{
    esp_mqtt_client_config_t mqtt_cfg = {
        .uri = MQTT_BROKER_URL,
        .event_handle = mqtt_event_handler,
        // .user_context = (void *)your_context
    };

    _config.mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    __device_status.datalink_status = DATALINK_STATUS_DISCONNECTED;
}

void start_mqtt(void)
{
    esp_mqtt_client_start(_config.mqtt_client);
}

void publish_device_status()
{
    if (__device_status.datalink_status != DATALINK_STATUS_CONNECTED)
        return;

    int msg_id = esp_mqtt_client_publish(_config.mqtt_client, MQTT_BODY_DETECTION_PUBLISH_TOPIC,
        __device_status.body_detected ? "true" : "false", 0, 1, 0);
    UNUSED(msg_id);
}

// For dev/debug only
static void process_body_detection_enabled_downlink(const char* data, int data_len)
{
    device_control_event event = {};

    if (strncasecmp(data, "true", data_len) == 0) {
        event.event_type = DEVICE_CONTROL_EVENT_BODY_DETECTION_ENABLED;
        device_control_send_event(&event);
        return;
    }

    if (strncasecmp(data, "false", data_len) == 0) {
        event.event_type = DEVICE_CONTROL_EVENT_BODY_DETECTION_DISABLED;
        device_control_send_event(&event);
        return;
    }

    ESP_LOGE(LOG_TAG_MQTT, "invalid body detection enabled received: %.*s", data_len, data);
}

static void process_body_detection_delay_downlink(const char* data, int data_len)
{
    char delay_str[data_len + 1];
    strncpy(delay_str, data, data_len)[data_len] = 0;
    int delay = atoi(delay_str);

    if (delay > 0) {
        device_control_event event = {};
        event.event_type = DEVICE_CONTROL_EVENT_BODY_DETECTION_DELAY_CHANGED;
        event.body_detection_delay_seconds = delay;
        device_control_send_event(&event);
    } else {
        ESP_LOGE(LOG_TAG_MQTT, "invalid body detection delay received: %d (%.*s)", delay, data_len, data);
    }
}

static void process_downlink_data(const char* topic, int topic_len, const char* data, int data_len)
{
    int result = 0;

    result = strncmp(topic, MQTT_CONFIG_BODY_DETECTION_ENABLED_TOPIC, topic_len);
    if (result == 0) {
        process_body_detection_enabled_downlink(data, data_len);
        return;
    }

    result = strncmp(topic, MQTT_CONFIG_BODY_DETECTION_DELAY_TOPIC, topic_len);
    if (result == 0) {
        process_body_detection_delay_downlink(data, data_len);
        return;
    }

    ESP_LOGE(LOG_TAG_MQTT, "unrecognized topic or broken data, topic: %.*s, payload: %.*s", topic_len, topic, data_len, data);
}

static void subscribe_mqtt_topics(esp_mqtt_client_handle_t client)
{
    esp_mqtt_client_subscribe(client, MQTT_CONFIG_SUBSCRIBE_TOPIC, 0);
}

void datalink_send_event(data_link_event *event)
{
    ESP_LOGE(LOG_TAG_MQTT, "data link event received. not impelemented yet");
}

void init_datalink(bool mqtt_enabled, bool azure_iot_enabled)
{
    _config.enable_mqtt = mqtt_enabled;
    _config.enable_azure_iot = azure_iot_enabled;

    _config.datalink_event_queue = xQueueCreate(20, sizeof(data_link_event));
    
    if (mqtt_enabled) {
        init_mqtt();
    }
}

void start_datalink(void)
{
    if (_config.enable_mqtt) {
        start_mqtt();
    }
}