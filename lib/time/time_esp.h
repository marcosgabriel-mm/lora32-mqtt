#ifndef TIME_ESP_H
#define TIME_ESP_H

#include <time.h>
#include "esp_err.h"

/**
 * @brief Sincroniza o tempo com servidor NTP
 * @return ESP_OK se sincronizado com sucesso
 */
esp_err_t sync_time();

/**
 * @brief Obtém timestamp em segundos desde epoch
 * @return Timestamp em segundos
 */
time_t get_epoch_time();

/**
 * @brief Obtém timestamp em milissegundos desde epoch
 * @return Timestamp em ms
 */
uint64_t get_epoch_time_ms();

/**
 * @brief Obtém timestamp em microsegundos desde epoch
 * @return Timestamp em µs
 */
uint64_t get_epoch_time_us();

/**
 * @brief Formata timestamp em microsegundos para string legível
 * @param timestamp_us Timestamp em microsegundos
 * @return String formatada (YYYY-MM-DD HH:MM:SS:mmm)
 */
char* formart_timestamp_us(uint64_t timestamp_us);

/**
 * @brief Obtém tempo formatado atual
 * @return String com tempo formatado
 */
char* get_format_time();

#endif // TIME_ESP_H