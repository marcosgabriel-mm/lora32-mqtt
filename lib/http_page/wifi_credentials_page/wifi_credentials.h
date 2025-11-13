#ifndef WIFI_CREDENTIALS_H
#define WIFI_CREDENTIALS_H

#include <esp_http_server.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    char ssid[32];
    char password[64];
    bool connected;
} wifi_credentials_t;

void start_webserver(void);
wifi_credentials_t wifi_credentials(void);

#ifdef __cplusplus
}
#endif

#endif // WIFI_CREDENTIALS_H