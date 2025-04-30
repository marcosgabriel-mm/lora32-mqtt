#ifndef TIME_ESP_H
#define TIME_ESP_H

#include <esp_err.h>

char* get_format_time();
esp_err_t sync_time();

#endif