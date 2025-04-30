#ifndef WIFI_ESP_H
#define WIFI_ESP_H

#include <esp_err.h>
#include <esp_event_base.h>
#include <freertos/event_groups.h>

void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
esp_err_t wifi_init(const char* ssid, const char* pass);
void wifi_scan();
void start_wifi_ap();

#endif
