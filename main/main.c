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

#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/gpio.h"
#include "esp_log.h"
#include "mqtt_client.h"
#include "nvs_flash.h"

#include "global.h"

#include "bodydetection.h"
#include "datalink.h"
#include "devicecontrollogic.h"
#include "led.h"
#include "status.h"
#include "tasks.h"
#include "wifi.h"

void app_main()
{
    ESP_LOGI(LOG_TAG_APP, "Starting up..");
    ESP_LOGI(LOG_TAG_APP, "Free memory: %d bytes", esp_get_free_heap_size());
    ESP_LOGI(LOG_TAG_APP, "IDF version: %s", esp_get_idf_version());

    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("MQTT_CLIENT", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT_TCP", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT_SSL", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT", ESP_LOG_VERBOSE);
    esp_log_level_set("OUTBOX", ESP_LOG_VERBOSE);

    // LED
    /*
    gpio_pad_select_gpio(12);
    gpio_set_direction(12, GPIO_MODE_OUTPUT);
    gpio_set_level(12, 1);

    gpio_pad_select_gpio(13);
    gpio_set_direction(13, GPIO_MODE_OUTPUT);
    gpio_set_level(13, 1);

    gpio_pad_select_gpio(14);
    gpio_set_direction(14, GPIO_MODE_OUTPUT);
    gpio_set_level(14, 1);

    gpio_pad_select_gpio(15);
    gpio_set_direction(15, GPIO_MODE_OUTPUT);
    gpio_set_level(15, 1);
*/

    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);

    init_led();
    init_device_control_logic();
    init_body_detection();
    init_wifi();
    init_mqtt();

    const unsigned char ssid[] = WIFI_SSID;
    const unsigned char cred[] = WIFI_PASS;
    set_wifi_security(WIFI_SEC_WPA_WPA2_PSK, ssid, sizeof ssid, cred, sizeof cred);

    start_device_control_logic();
    start_body_detection();
    start_wifi();

    connect_wifi();
    start_mqtt();

    // Start task to collect fan info, idle task
    xTaskCreate(collect_device_status_task, "collect_device_status_task", 4096, NULL, 0, NULL);
    // Start task to publish device status, idle task
    xTaskCreate(publish_device_status_task, "publish_device_status_task", 4096, NULL, 0, NULL);
}
