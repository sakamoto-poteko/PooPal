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


#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "global.h"
#include "status.h"
#include "datalink.h"
#include "bodydetection.h"

#include "status.h"

DeviceStatus __device_status = {};

void collect_device_status_task(void *pvParameters)
{
    UNUSED(pvParameters);
    const TickType_t xDelay = DEVICE_STATUS_COLLECT_INTERVAL_MS / portTICK_PERIOD_MS;

    while (true) {
        __device_status.body_detected = get_body_detected();
        vTaskDelay(xDelay);
    }
}

void publish_device_status_task(void *pvParameters)
{
    UNUSED(pvParameters);
    const TickType_t xDelay = DEVICE_STATUS_PUBLISH_INTERVAL_MS / portTICK_PERIOD_MS;

    while (true) {
        publish_device_status();
        vTaskDelay(xDelay);
    }
}
