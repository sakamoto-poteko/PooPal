// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license.

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

#include <stdio.h>
#include <stdlib.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_system.h"

#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/platform.h"
#include "azure_c_shared_utility/shared_util_options.h"
#include "azure_c_shared_utility/threadapi.h"
#include "iothub_client.h"
#include "iothub_client_options.h"
#include "iothub_device_client_ll.h"
#include "iothub_message.h"
#include "iothubtransportmqtt.h"

#include "global.h"
#include "creddef.h"

#ifdef MBED_BUILD_TIMESTAMP
#define SET_TRUSTED_CERT_IN_SAMPLES
#endif // MBED_BUILD_TIMESTAMP

#ifdef SET_TRUSTED_CERT_IN_SAMPLES
#include "certs.h"
#endif // SET_TRUSTED_CERT_IN_SAMPLES


typedef struct aziot_config_t {
    IOTHUB_CLIENT_LL_HANDLE iothub_client_handle;
} aziot_config;

static aziot_config _config;
static const char* aziothub_connection_string = AZIOTHUB_CONNSTR;


static IOTHUBMESSAGE_DISPOSITION_RESULT aziot_receive_message_callback(IOTHUB_MESSAGE_HANDLE message, void* user_context_callback)
{
    UNUSED(user_context_callback);

    const char* message_id;
    const char* correlation_id;

    // Message properties
    if ((message_id = IoTHubMessage_GetMessageId(message)) == NULL) {
        message_id = "<null>";
    }

    if ((correlation_id = IoTHubMessage_GetCorrelationId(message)) == NULL) {
        correlation_id = "<null>";
    }

    const char* buffer;
    size_t size;
    // Message content
    if (IoTHubMessage_GetByteArray(message, (const unsigned char**)&buffer, &size) != IOTHUB_MESSAGE_OK) {
        ESP_LOGE(LOG_TAG_AZIOT, "unable to retrieve downlink message");
    } else {
        ESP_LOGI(LOG_TAG_AZIOT, "received downlink message, msg id: %s, correlation id: %s, size %d", message_id, correlation_id, size);
        ESP_LOGE(LOG_TAG_AZIOT, "downlink message processing not implemented");
    }

    MAP_HANDLE map_properties = IoTHubMessage_Properties(message);
    if (map_properties != NULL) {
        const char* const* keys;
        const char* const* values;
        size_t property_count = 0;
        if (Map_GetInternals(map_properties, &keys, &values, &property_count) != MAP_OK) {
        }
    }

    return IOTHUBMESSAGE_ACCEPTED;
}

static void aziot_message_sent_confirmation(IOTHUB_CLIENT_CONFIRMATION_RESULT result, void* user_context_callback)
{
    IOTHUB_MESSAGE_HANDLE handle = (IOTHUB_MESSAGE_HANDLE)user_context_callback;

    if (result == IOTHUB_CLIENT_CONFIRMATION_OK) {
        ESP_LOGI(LOG_TAG_AZIOT, "confirmation received for a msg, result = %s\r\n", MU_ENUM_TO_STRING(IOTHUB_CLIENT_CONFIRMATION_RESULT, result));
    }
    IoTHubMessage_Destroy(handle);
}

void aziot_connection_status_callback(IOTHUB_CLIENT_CONNECTION_STATUS result, IOTHUB_CLIENT_CONNECTION_STATUS_REASON reason, void* user_context_callback)
{
    ESP_LOGI(LOG_TAG_AZIOT, "status changed to: %s, reason: %s",
        MU_ENUM_TO_STRING(IOTHUB_CLIENT_CONNECTION_STATUS, result),
        MU_ENUM_TO_STRING(IOTHUB_CLIENT_CONNECTION_STATUS_REASON, reason));
}

static bool aziot_send_core(IOTHUB_MESSAGE_HANDLE message_handle)
{
    if (IoTHubClient_LL_SendEventAsync(_config.iothub_client_handle, message_handle, aziot_message_sent_confirmation, message_handle) != IOTHUB_CLIENT_OK) {
        ESP_LOGE(LOG_TAG_AZIOT, "failed to send message");
        return false;
    } else {
        ESP_LOGI(LOG_TAG_AZIOT, "message scheduled for transmission");
    }
    IoTHubClient_LL_DoWork(_config.iothub_client_handle);
    return true;
}

bool aziot_send_str(const char* data)
{
    IOTHUB_MESSAGE_HANDLE message_handle = IoTHubMessage_CreateFromString(data);
    if (message_handle == NULL) {
        ESP_LOGE(LOG_TAG_AZIOT, "failed to create message in send");
        return false;
    }

    return aziot_send_core(message_handle);
}

bool aziot_send_bin(const uint8_t* data, size_t len)
{
    IOTHUB_MESSAGE_HANDLE message_handle = IoTHubMessage_CreateFromByteArray(data, len);
    if (message_handle == NULL) {
        ESP_LOGE(LOG_TAG_AZIOT, "failed to create message in send");
        return false;
    }

    return aziot_send_core(message_handle);
}

bool aziot_init(void)
{
    IOTHUB_CLIENT_LL_HANDLE client = IoTHubClient_LL_CreateFromConnectionString(aziothub_connection_string, MQTT_Protocol);

    if (client == NULL) {
        ESP_LOGE(LOG_TAG_AZIOT, "failed to create client from conn string");
        return false;
    } else {
        _config.iothub_client_handle = client;
        ESP_LOGI(LOG_TAG_AZIOT, "created client from conn string");
    }

    bool traceOn = true;
    IoTHubClient_LL_SetOption(client, OPTION_LOG_TRACE, &traceOn);

    IoTHubDeviceClient_LL_SetRetryPolicy(client, IOTHUB_CLIENT_RETRY_INTERVAL, 0);

    IoTHubClient_LL_SetConnectionStatusCallback(client, aziot_connection_status_callback, NULL);

    // Setting the Trusted Certificate.  This is only necessary on system with without
    // built in certificate stores.
#ifdef SET_TRUSTED_CERT_IN_SAMPLES
    IoTHubDeviceClient_LL_SetOption(client, OPTION_TRUSTED_CERT, certificates);
#endif // SET_TRUSTED_CERT_IN_SAMPLES

    /* Setting Message call back, so we can receive Commands. */
    int receiveContext = 0;
    if (IoTHubClient_LL_SetMessageCallback(client, aziot_receive_message_callback, &receiveContext) != IOTHUB_CLIENT_OK) {
        ESP_LOGE(LOG_TAG_AZIOT, "failed to set message callback");
        goto aziot_init_failed;
    }

    return true;

aziot_init_failed:
    IoTHubClient_LL_Destroy(client);
    return false;
}

static void aziot_loop_task(void* unused)
{
    const TickType_t xDelay = 100 / portTICK_PERIOD_MS;

    while (true) {
        IoTHubClient_LL_DoWork(_config.iothub_client_handle);
        vTaskDelay(xDelay);
    }
}

void aziot_start(void)
{
    xTaskCreate(aziot_loop_task, "aziot_loop_task", 8192, NULL, 0, NULL);
}