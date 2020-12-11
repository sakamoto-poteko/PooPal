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

#include <math.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

#include "esp_event.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_wifi.h"

#include "lwip/err.h"
#include "lwip/sys.h"

#include "global.h"

#include "devicecontrollogic.h"
#include "status.h"
#include "wifi.h"

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t wifi_event_group;

const int WIFI_CONNECTED_BIT = BIT0;

static int retry_count;
static bool disconnect_instr;

static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    device_control_event evt;

    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) { // started
        ESP_LOGI(LOG_TAG_WIFI, "WiFi started");
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) { // disconnected
        xEventGroupClearBits(wifi_event_group, WIFI_CONNECTED_BIT);
        __device_status.wifi_status = WIFI_STATUS_DISCONNECTED;

        const wifi_event_sta_disconnected_t *event = event_data;
        char ssid[33];
        strncpy(ssid, (const char *)event->ssid, event->ssid_len)[event->ssid_len] = 0;
        ESP_LOGI(LOG_TAG_WIFI, "WiFi disconnected: SSID %s, reason: %u", ssid, event->reason);

        evt.event_type = DEVICE_CONTROL_EVENT_WIFI_DISCONNECTED;
        device_control_send_event(&evt);

        while (retry_count < WIFI_CONNECT_MAXIMUM_RETRY && !disconnect_instr) {
            ++retry_count;
            ESP_LOGI(LOG_TAG_WIFI, "WiFi reconnecting, retry %d...", retry_count);
            esp_wifi_connect();
        }

        if (disconnect_instr) {
            disconnect_instr = false;
        }
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_CONNECTED) { // connected
        evt.event_type = DEVICE_CONTROL_EVENT_WIFI_ASSOCIATED;
        device_control_send_event(&evt);
        ESP_LOGI(LOG_TAG_WIFI, "WiFi connected but IP not allocated yet");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) { // IP acquired
        retry_count = 0;
        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
        __device_status.wifi_status = WIFI_STATUS_CONNECTED;
        evt.event_type = DEVICE_CONTROL_EVENT_WIFI_CONNECTED;
        device_control_send_event(&evt);

        ip_event_got_ip_t* event = (ip_event_got_ip_t*)event_data;
        ESP_LOGI(LOG_TAG_WIFI, "WiFi IP acquired: " IPSTR, IP2STR(&event->ip_info.ip));
    }
}

void init_wifi(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, NULL));

    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_FLASH));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

    __device_status.wifi_status = WIFI_STATUS_DISCONNECTED;
}

void start_wifi(void)
{
    ESP_ERROR_CHECK(esp_wifi_start());
    retry_count = 0;
    ESP_LOGI(LOG_TAG_WIFI, "starting WiFi...");

    device_control_event e;
    e.event_type = DEVICE_CONTROL_EVENT_WIFI_DISCONNECTED;
    device_control_send_event(&e);
}

void disconnect_wifi(void)
{
    ESP_ERROR_CHECK(esp_wifi_disconnect());
    disconnect_instr = true;
    retry_count = 0;
    ESP_LOGI(LOG_TAG_WIFI, "disconnecting WiFi...");
}

void connect_wifi(void)
{
    wifi_ap_record_t apinfo = {};
    esp_err_t con = esp_wifi_sta_get_ap_info(&apinfo);
    if (con == ESP_OK) { // Already connected
        return;
    }

    device_control_event evt;
    evt.event_type = DEVICE_CONTROL_EVENT_WIFI_CONNECTING;
    device_control_send_event(&evt);

    esp_err_t ok = esp_wifi_connect();
    if (ok == ESP_OK) {
        ESP_LOGI(LOG_TAG_WIFI, "connecting WiFi...");
    } else {
        ESP_LOGE(LOG_TAG_WIFI, "WiFi connect failed: %s", esp_err_to_name(ok));
    }
}

void set_wifi_security(wifi_security security, const uint8_t* ssid, size_t ssid_len, const uint8_t* credentials, size_t credentials_len)
{
    ESP_LOGI(LOG_TAG_WIFI, "setting WiFi security mode 0x%0X, ssid %.*s", security, ssid_len, ssid);

    wifi_config_t wifi_config = {
        .sta = {
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
            .pmf_cfg = {
                .capable = true,
                .required = false } }
    };

    switch (security) {
    case WIFI_SEC_WEP:
    case WIFI_SEC_WPA_WPA2_PSK:
        strncpy((char*)wifi_config.sta.password, (const char*)credentials, MIN(credentials_len, WIFI_PASSWORD_MAX_LEN))[WIFI_PASSWORD_MAX_LEN - 1] = 0;
        // fall through
    case WIFI_SEC_OPEN:
        strncpy((char*)wifi_config.sta.ssid, (const char*)ssid, MIN(ssid_len, WIFI_SSID_MAX_LEN))[WIFI_SSID_MAX_LEN - 1] = 0;
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
        ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
        break;
    default:
        ESP_LOGE(LOG_TAG_WIFI, "unrecognized wifi_security");
        abort();
    }

    ESP_LOGI(LOG_TAG_WIFI, "WiFi security set");
}

void reconnect_wifi()
{
    disconnect_wifi();
    connect_wifi();
}
