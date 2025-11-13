#ifndef LORA_ESP_H
#define LORA_ESP_H

#include "esp_err.h"
/**
 * @brief Inicializa o LoRa como mestre/receptor
 * @return ESP_OK se inicializado com sucesso, ESP_FAIL caso contrário
 */
esp_err_t init_lora_master(void);

/**
 * @brief Task para recepção de mensagens LoRa
 * @param pvParameters Parâmetros da task (não utilizados)
 */
void lora_rx_task(void *pvParameters);

#endif // LORA_ESP_H