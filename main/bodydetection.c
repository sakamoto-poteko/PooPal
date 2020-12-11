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
#include "freertos/queue.h"
#include "freertos/task.h"

#include "driver/gpio.h"
#include "esp_log.h"

#include "global.h"

#include "bodydetection.h"
#include "devicecontrollogic.h"

static xQueueHandle body_detection_gpio_queue = NULL;
static bool body_detected;

static void IRAM_ATTR body_detection_isr_handler(void* arg)
{
    UNUSED(arg);
    int level = gpio_get_level(BODY_DETECTION_PIN);
    xQueueSendFromISR(body_detection_gpio_queue, &level, NULL);
}

static void body_detection_task(void* arg)
{
    UNUSED(arg);
    int level;
    for (;;) {
        if (xQueueReceive(body_detection_gpio_queue, &level, portMAX_DELAY)) {
            if (BODY_DETECTION_LOW_ACTIVE) { // Inverted
                body_detected = !level;
            } else {
                body_detected = level;
            }

            device_control_event event = {
                .event_type = DEVICE_CONTROL_EVENT_BODY_DETECTION_TRIGGERED,
                .body_detected = level
            };
            device_control_send_event(&event);
            ESP_LOGI(LOG_TAG_BODY_DETECTION, "body detection pin level %s", level ? "hi" : "lo");
        }
    }
}

void init_body_detection()
{
    gpio_pad_select_gpio(BODY_DETECTION_PIN);
    gpio_set_direction(BODY_DETECTION_PIN, GPIO_MODE_INPUT);
    gpio_set_pull_mode(BODY_DETECTION_PIN, GPIO_FLOATING);
    gpio_set_intr_type(BODY_DETECTION_PIN, GPIO_INTR_ANYEDGE);

    body_detection_gpio_queue = xQueueCreate(10, sizeof(int));
}

void start_body_detection()
{
    gpio_isr_handler_add(BODY_DETECTION_PIN, body_detection_isr_handler, NULL);

    xTaskCreate(body_detection_task, "body_detection_task", 2048, NULL, 10, NULL);
}

int get_body_detected()
{
    return body_detected;
}
