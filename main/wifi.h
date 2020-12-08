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

#ifndef WIFI_H
#define WIFI_H

#include <stdint.h>

#include "global.h"

typedef enum wifi_security_t {
    WIFI_SEC_OPEN = 0x0,
    WIFI_SEC_WEP = 0x1,
    WIFI_SEC_WPA_WPA2_PSK = 0x2,
    //    WIFI_SEC_WPA_WPA2_ENT,
} wifi_security;

void init_wifi(void);
void start_wifi(void);
void disconnect_wifi(void);
void connect_wifi(void);
void reconnect_wifi(void);
void set_wifi_security(wifi_security security, const uint8_t *ssid, size_t ssid_len, const uint8_t *credentials, size_t credentials_len);

#endif // WIFI_H
