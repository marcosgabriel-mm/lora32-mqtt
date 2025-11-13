#ifndef TIME_ESP_H
#define TIME_ESP_H

#include <esp_err.h>
#include <ctime>

#ifdef __cplusplus
extern "C" {
#endif

char* get_format_time(void);
esp_err_t sync_time(void);
time_t get_epoch_time(void);
int64_t get_epoch_time_ms(void);

#ifdef __cplusplus
}
#endif

#endif // TIME_ESP_H