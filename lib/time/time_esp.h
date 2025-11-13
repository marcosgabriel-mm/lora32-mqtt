#ifndef TIME_ESP_H
#define TIME_ESP_H

#include <esp_err.h>
#include <ctime>

char* get_format_time();
char* formart_timestamp_us(int64_t timestamp_us);

esp_err_t sync_time();
time_t get_epoch_time();

int64_t get_epoch_time_ms();
int64_t get_epoch_time_us();

#endif