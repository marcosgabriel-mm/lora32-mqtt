#ifndef LORA_ESP_H
#define LORA_ESP_H

#include "esp_err.h"
#include <sys/time.h>

// Tipos de mensagens LoRa
typedef enum {
    LORA_MSG_TIME_REQUEST = 0x01,    // Requisição de tempo
    LORA_MSG_TIME_RESPONSE = 0x02,   // Resposta com tempo do servidor
    LORA_MSG_DATA = 0x03,            // Dados normais (string)
    LORA_MSG_TIMESTAMP = 0x04,       // Timestamp sincronizado
    LORA_MSG_SENSOR_DATA = 0x05      // Dados de sensores
} lora_msg_type_t;

// Estrutura para mensagem de sincronização de tempo
typedef struct __attribute__((packed)) {
    uint8_t msg_type;           // Tipo da mensagem
    uint64_t t1;                // Timestamp do cliente ao enviar requisição (em µs)
    uint64_t t2;                // Timestamp do servidor ao receber requisição (em µs)
    uint64_t t3;                // Timestamp do servidor ao enviar resposta (em µs)
    uint64_t t4;                // Timestamp do cliente ao receber resposta (em µs) (preenchido localmente)
} lora_time_sync_msg_t;

// Estrutura para mensagem genérica
typedef struct __attribute__((packed)) {
    uint8_t msg_type;
    char payload[254];
} lora_msg_t;

// Estrutura para mensagem de timestamp
typedef struct __attribute__((packed)) {
    uint8_t msg_type;
    uint64_t timestamp_us;
    char description[50];
} lora_timestamp_msg_t;

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

/**
 * @brief Inicializa o LoRa como escravo/transmissor
 * @return ESP_OK se inicializado com sucesso, ESP_FAIL caso contrário
 */
esp_err_t init_lora_slave();

/**
 * @brief Solicita sincronização de tempo via LoRa
 * @return ESP_OK se sincronizado com sucesso
 */
esp_err_t lora_request_timesync();

/**
 * @brief Envia resposta de sincronização de tempo
 * @param request Estrutura com a requisição recebida
 * @return ESP_OK se enviado com sucesso
 */
esp_err_t lora_send_time_response(lora_time_sync_msg_t *request);

/**
 * @brief Envia uma mensagem de dados via LoRa
 * @param message String com a mensagem a enviar
 * @return ESP_OK se enviado com sucesso
 */
esp_err_t lora_send_data(const char* message);

/**
 * @brief Envia o timestamp atual via LoRa
 * @param description Descrição do timestamp (opcional)
 * @return ESP_OK se enviado com sucesso
 */
esp_err_t lora_send_timestamp(const char* description);

#endif // LORA_ESP_H