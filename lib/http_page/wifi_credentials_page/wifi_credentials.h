#define WIFI_CREDENTIALS_H
#ifdef WIFI_CREDENTIALS_H

#pragma once
#include <esp_http_server.h>

typedef struct {
    char ssid[32];
    char password[64];
    bool connected;
} wifi_credentials_t;

void start_webserver(void);
wifi_credentials_t wifi_credentials(void);

#endif