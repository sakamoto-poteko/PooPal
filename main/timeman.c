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
#include <sys/time.h>
#include <time.h>

#include "esp_log.h"
#include "esp_sntp.h"

#include "global.h"

#include "timeman.h"

static bool _is_time_set = false;
static bool _timeman_started = false;

bool timeman_is_time_set()
{
    // if the flag is set, i.e. time is set, it is set
    if (_is_time_set) {
        return true;
    } else {
        // else we'll need to check if it's set
        time_t now;
        struct tm timeinfo;
        time(&now);
        localtime_r(&now, &timeinfo);

        // Is time set? If not, tm_year will be (1970 - 1900).
        if (timeinfo.tm_year < (2020 - 1900)) {
            // time is not set yet
            return false;
        } else {
            _is_time_set = true;
            return true;
        }
    }
}

static void time_sync_notification_cb(struct timeval* tv)
{
    sntp_sync_status_t sync_status = sntp_get_sync_status();
    switch (sync_status) {
    case SNTP_SYNC_STATUS_RESET:
        ESP_LOGI(LOG_TAG_TIMEMAN, "waiting for system time to be set...");
        break;
    case SNTP_SYNC_STATUS_COMPLETED: {
        time_t now = 0;
        struct tm timeinfo = { 0 };
        time(&now);
        localtime_r(&now, &timeinfo);
        timeman_is_time_set();
        ESP_LOGI(LOG_TAG_TIMEMAN, "system time synced: %d-%02d-%02d %02d:%02d:%02d",
            timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday, timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
        break;
    }
    case SNTP_SYNC_STATUS_IN_PROGRESS:
        ESP_LOGI(LOG_TAG_TIMEMAN, "syncing system time...");
        break;
    }
}

void timeman_start()
{
    if (_timeman_started) {
        return;
    }

    ESP_LOGI(LOG_TAG_TIMEMAN, "initializing SNTP");
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "time.ustc.edu.cn");
    sntp_setservername(1, "ntp.tuna.tsinghua.edu.cn");
    sntp_setservername(2, "time.windows.com");
    sntp_setservername(3, "pool.ntp.org");
    _Static_assert(4 < SNTP_MAX_SERVERS, "too many NTP servers");
    sntp_set_time_sync_notification_cb(time_sync_notification_cb);
    sntp_init();

    _timeman_started = true;
}