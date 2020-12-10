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
#include "freertos/event_groups.h"
#include "freertos/task.h"

#include "driver/ledc.h"
#include "esp_err.h"
#include "esp_log.h"

#include "global.h"

#include "led.h"

#define LED1_TIMER LEDC_TIMER_0

#define LED1_CHANNEL LEDC_CHANNEL_0

#define LED_PWM_FREQUENCY 1000
//#define LED_DUTYCYCLE_FULL (1 << (LEDC_TIMER_15_BIT))   // Too bright
#define LED_DUTYCYCLE_FULL ((1 << (LEDC_TIMER_15_BIT)) >> 2)

#define LED_CONTROL_EVENT_GROUP_BIT(x) (1 << (x))

static EventGroupHandle_t led_control_event_group;

static ledc_channel_config_t ledc_channel[LED_COUNT] = {
    { .channel = LED1_CHANNEL,
        .duty = 0,
        .gpio_num = LED_1_PIN,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_sel = LED1_TIMER },
};

typedef enum {
    LED_OFF,
    LED_ON,
    LED_FLASH,
    LED_FADE_IN,
    LED_FADE_OUT,
    LED_FADE_IN_OUT,
} LedControl;

typedef struct {
    LedControl ctrl;
    int on_time_ms;
    int off_time_ms;
} LedInfo;

static LedInfo led_info[LED_COUNT];
static int led_status[LED_COUNT];

void led_control(int led_idx)
{
    switch (led_info[led_idx].ctrl) {
    case LED_OFF:
        ledc_set_duty(ledc_channel[led_idx].speed_mode, ledc_channel[led_idx].channel, 0);
        ledc_update_duty(ledc_channel[led_idx].speed_mode, ledc_channel[led_idx].channel);
        xEventGroupWaitBits(led_control_event_group, LED_CONTROL_EVENT_GROUP_BIT(led_idx), pdTRUE, pdFALSE, portMAX_DELAY);
        break;
    case LED_ON:
        ledc_set_duty(ledc_channel[led_idx].speed_mode, ledc_channel[led_idx].channel, LED_DUTYCYCLE_FULL);
        ledc_update_duty(ledc_channel[led_idx].speed_mode, ledc_channel[led_idx].channel);
        xEventGroupWaitBits(led_control_event_group, LED_CONTROL_EVENT_GROUP_BIT(led_idx), pdTRUE, pdFALSE, portMAX_DELAY);
        break;
    case LED_FLASH: {
        int dutycycle = 0;
        if (led_status[led_idx]) {
            dutycycle = 0;
            led_status[led_idx] = false;
        } else {
            dutycycle = LED_DUTYCYCLE_FULL;
            led_status[led_idx] = true;
        }
        ledc_set_duty(ledc_channel[led_idx].speed_mode, ledc_channel[led_idx].channel, dutycycle);
        ledc_update_duty(ledc_channel[led_idx].speed_mode, ledc_channel[led_idx].channel);
        TickType_t wait = led_info[led_idx].on_time_ms / portTICK_PERIOD_MS;
        xEventGroupWaitBits(led_control_event_group, LED_CONTROL_EVENT_GROUP_BIT(led_idx), pdTRUE, pdFALSE, wait);
        break;
    }
    case LED_FADE_IN: {
        ledc_set_duty(ledc_channel[led_idx].speed_mode, ledc_channel[led_idx].channel, 0);
        ledc_update_duty(ledc_channel[led_idx].speed_mode, ledc_channel[led_idx].channel);
        ledc_set_fade_with_time(ledc_channel[led_idx].speed_mode,
            ledc_channel[led_idx].channel, LED_DUTYCYCLE_FULL, led_info[led_idx].on_time_ms);
        ledc_fade_start(ledc_channel[led_idx].speed_mode,
            ledc_channel[led_idx].channel, LEDC_FADE_NO_WAIT);
        TickType_t wait = led_info[led_idx].on_time_ms / portTICK_PERIOD_MS;
        xEventGroupWaitBits(led_control_event_group, LED_CONTROL_EVENT_GROUP_BIT(led_idx), pdTRUE, pdFALSE, wait);
        break;
    }
    case LED_FADE_OUT: {
        ledc_set_duty(ledc_channel[led_idx].speed_mode, ledc_channel[led_idx].channel, LED_DUTYCYCLE_FULL);
        ledc_update_duty(ledc_channel[led_idx].speed_mode, ledc_channel[led_idx].channel);
        ledc_set_fade_with_time(ledc_channel[led_idx].speed_mode,
            ledc_channel[led_idx].channel, 0, led_info[led_idx].off_time_ms);
        ledc_fade_start(ledc_channel[led_idx].speed_mode,
            ledc_channel[led_idx].channel, LEDC_FADE_NO_WAIT);
        TickType_t wait = led_info[led_idx].off_time_ms / portTICK_PERIOD_MS;
        xEventGroupWaitBits(led_control_event_group, LED_CONTROL_EVENT_GROUP_BIT(led_idx), pdTRUE, pdFALSE, wait);
        break;
    }
    case LED_FADE_IN_OUT: {
        ledc_set_duty(ledc_channel[led_idx].speed_mode, ledc_channel[led_idx].channel, 0);
        ledc_update_duty(ledc_channel[led_idx].speed_mode, ledc_channel[led_idx].channel);
        // In
        ledc_set_fade_with_time(ledc_channel[led_idx].speed_mode,
            ledc_channel[led_idx].channel, LED_DUTYCYCLE_FULL, led_info[led_idx].on_time_ms);
        ledc_fade_start(ledc_channel[led_idx].speed_mode,
            ledc_channel[led_idx].channel, LEDC_FADE_NO_WAIT);
        TickType_t wait_in = led_info[led_idx].on_time_ms / portTICK_PERIOD_MS;
        xEventGroupWaitBits(led_control_event_group, LED_CONTROL_EVENT_GROUP_BIT(led_idx), pdTRUE, pdFALSE, wait_in);
        // Out
        ledc_set_fade_with_time(ledc_channel[led_idx].speed_mode,
            ledc_channel[led_idx].channel, 0, led_info[led_idx].on_time_ms);
        ledc_fade_start(ledc_channel[led_idx].speed_mode,
            ledc_channel[led_idx].channel, LEDC_FADE_NO_WAIT);
        TickType_t wait_out = led_info[led_idx].on_time_ms / portTICK_PERIOD_MS;
        xEventGroupWaitBits(led_control_event_group, LED_CONTROL_EVENT_GROUP_BIT(led_idx), pdTRUE, pdFALSE, wait_out);
        break;
    }
    default:
        break;
    }
}

void task_led_control_led1(void* pv)
{
    UNUSED(pv);
    while (true) {
        led_control(LED_1);
    }
}

void init_led()
{
    /*
     * Prepare and set configuration of timers
     * that will be used by LED Controller
     */
    ledc_timer_config_t ledc_timer = {
        .duty_resolution = LEDC_TIMER_15_BIT, // resolution of PWM duty
        .freq_hz = LED_PWM_FREQUENCY, // frequency of PWM signal
        .speed_mode = LEDC_LOW_SPEED_MODE, // timer mode
        .timer_num = LED1_TIMER // timer index
    };
    ledc_timer_config(&ledc_timer);

    /*
     * Prepare individual configuration
     * for each channel of LED Controller
     * by selecting:
     * - controller's channel number
     * - output duty cycle, set initially to 0
     * - GPIO number where LED is connected to
     * - speed mode, either high or low
     * - timer servicing selected channel
     *   Note: if different channels use one timer,
     *         then frequency and bit_num of these channels
     *         will be the same
     */

    // Set LED Controller with previously prepared configuration
    for (int ch = 0; ch < LED_COUNT; ch++) {
        ledc_channel_config(&ledc_channel[ch]);
    }

    // Initialize fade service.
    ledc_fade_func_install(0);

    led_control_event_group = xEventGroupCreate();

    xTaskCreate(task_led_control_led1, "task_led_control_1", 1024, NULL, 0, NULL);
}

void set_led_on(Led led)
{
    led_info[led].ctrl = LED_ON;
    led_status[led] = 1;
    xEventGroupSetBits(led_control_event_group, LED_CONTROL_EVENT_GROUP_BIT(led));
    ESP_LOGI(LOG_TAG_LED, "setting led %d on", led);
}

void set_led_off(Led led)
{
    led_info[led].ctrl = LED_OFF;
    led_status[led] = 0;
    xEventGroupSetBits(led_control_event_group, LED_CONTROL_EVENT_GROUP_BIT(led));
    ESP_LOGI(LOG_TAG_LED, "setting led %d off", led);
}

void set_led_flash(Led led, int on_time_ms)
{
    led_info[led].ctrl = LED_FLASH;
    led_info[led].on_time_ms = on_time_ms;
    led_status[led] = 0;
    xEventGroupSetBits(led_control_event_group, LED_CONTROL_EVENT_GROUP_BIT(led));
    ESP_LOGI(LOG_TAG_LED, "setting led %d flash, on %dms", led, on_time_ms);
}

void set_led_on_off(Led led, int on_off)
{
    if (on_off) {
        set_led_on(led);
    } else {
        set_led_off(led);
    }
}

void set_led_fade_out(Led led, int off_time_ms)
{
    led_info[led].ctrl = LED_FADE_OUT;
    led_info[led].off_time_ms = off_time_ms;
    xEventGroupSetBits(led_control_event_group, LED_CONTROL_EVENT_GROUP_BIT(led));
    ESP_LOGI(LOG_TAG_LED, "setting led %d fade out, off %dms", led, off_time_ms);
}

void set_led_fade_in_out(Led led, int on_time_ms, int off_time_ms)
{
    led_info[led].ctrl = LED_FADE_IN_OUT;
    led_info[led].on_time_ms = on_time_ms;
    led_info[led].off_time_ms = off_time_ms;
    xEventGroupSetBits(led_control_event_group, LED_CONTROL_EVENT_GROUP_BIT(led));
    ESP_LOGI(LOG_TAG_LED, "setting led %d fade in/out, on %dms, off %dms", led, on_time_ms, off_time_ms);
}

void set_led_fade_in(Led led, int on_time_ms)
{
    led_info[led].ctrl = LED_FADE_IN;
    led_info[led].on_time_ms = on_time_ms;
    xEventGroupSetBits(led_control_event_group, LED_CONTROL_EVENT_GROUP_BIT(led));
    ESP_LOGI(LOG_TAG_LED, "setting led %d fade in, on %dms", led, on_time_ms);
}
