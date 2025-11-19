#include "ra01s.h"
#include <esp_log.h>
#include <mqtt_esp.h>
#include "lora_esp.h"
#include "time_esp.h"
#include <string.h>
#include <sys/time.h>
#include <cJSON.h>

static const char* TAG = "LoRa";

// Solicita sincronização de tempo (SLAVE)
esp_err_t lora_request_timesync()
{
    ESP_LOGI(TAG, "Requesting time sync via LoRa...");
    
    lora_time_sync_msg_t msg = {
        .msg_type = LORA_MSG_TIME_REQUEST,
        .t1 = get_epoch_time_us(),  // T1: momento do envio (em µs)
        .t2 = 0,
        .t3 = 0,
        .t4 = 0
    };
    
    if (LoRaSend((uint8_t*)&msg, sizeof(msg), SX126x_TXMODE_SYNC) == false) {
        ESP_LOGE(TAG, "Failed to send time sync request");
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "Time sync request sent at T1=%llu µs", msg.t1);
    return ESP_OK;
}

// Envia resposta de tempo (MASTER)
esp_err_t lora_send_time_response(lora_time_sync_msg_t *request)
{
    lora_time_sync_msg_t response = {
        .msg_type = LORA_MSG_TIME_RESPONSE,
        .t1 = request->t1,                      // T1: do cliente
        .t2 = request->t2,                      // T2: recepção no servidor
        .t3 = get_epoch_time_us(),              // T3: envio da resposta (em µs)
        .t4 = 0                                 // T4: será preenchido pelo cliente
    };
    
    if (LoRaSend((uint8_t*)&response, sizeof(response), SX126x_TXMODE_SYNC) == false) {
        ESP_LOGE(TAG, "Failed to send time sync response");
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "Time sync response sent: T1=%llu T2=%llu T3=%llu µs", 
             response.t1, response.t2, response.t3);
    return ESP_OK;
}

// Processa resposta de tempo e ajusta relógio (SLAVE)
static void process_time_response(lora_time_sync_msg_t *response)
{
    uint64_t t4 = get_epoch_time_us(); // T4: momento da recepção (em µs)
    
    // Cálculo do offset e delay de rede
    // Offset = ((T2 - T1) + (T3 - T4)) / 2
    // Delay = (T4 - T1) - (T3 - T2)
    
    int64_t offset = ((int64_t)(response->t2 - response->t1) + 
                      (int64_t)(response->t3 - t4)) / 2;
    
    int64_t round_trip_delay = (t4 - response->t1) - (response->t3 - response->t2);
    
    ESP_LOGI(TAG, "Time sync complete:");
    ESP_LOGI(TAG, "  T1=%llu µs (%s)", response->t1, formart_timestamp_us(response->t1));
    ESP_LOGI(TAG, "  T2=%llu µs (%s)", response->t2, formart_timestamp_us(response->t2));
    ESP_LOGI(TAG, "  T3=%llu µs (%s)", response->t3, formart_timestamp_us(response->t3));
    ESP_LOGI(TAG, "  T4=%llu µs (%s)", t4, formart_timestamp_us(t4));
    ESP_LOGI(TAG, "  Offset: %lld µs (%.3f ms)", offset, offset / 1000.0);
    ESP_LOGI(TAG, "  Round-trip delay: %lld µs (%.3f ms)", round_trip_delay, round_trip_delay / 1000.0);
    
    // Ajusta o relógio do sistema se offset > 1ms (1000µs)
    if (llabs(offset) > 10000) {
        struct timeval tv;
        gettimeofday(&tv, NULL);
        
        int64_t new_time_us = (tv.tv_sec * 1000000LL + tv.tv_usec) + offset;
        tv.tv_sec = new_time_us / 1000000;
        tv.tv_usec = new_time_us % 1000000;
        
        settimeofday(&tv, NULL);
        setenv("TZ", "BRT3", 1);
        tzset();

        ESP_LOGI(TAG, "System time adjusted by %lld µs (%.3f ms)", offset, offset / 1000.0);
        ESP_LOGI(TAG, "New time: %s", formart_timestamp_us(new_time_us));
    } else {
        ESP_LOGI(TAG, "Time offset too small, no adjustment needed");
    }
}

// Envia uma mensagem de dados via LoRa
esp_err_t lora_send_data(const char* message)
{
    lora_msg_t msg;
    msg.msg_type = LORA_MSG_DATA;
    strncpy(msg.payload, message, sizeof(msg.payload) - 1);
    msg.payload[sizeof(msg.payload) - 1] = '\0';
    
    int txLen = 1 + strlen(msg.payload) + 1; // msg_type + payload + null terminator
    
    if (LoRaSend((uint8_t*)&msg, txLen, SX126x_TXMODE_SYNC) == false) {
        ESP_LOGE(TAG, "Failed to send data message");
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "Data message sent: [%s]", msg.payload);
    return ESP_OK;
}

// Envia o timestamp atual via LoRa
esp_err_t lora_send_timestamp(const char* description)
{
    lora_timestamp_msg_t msg;
    msg.msg_type = LORA_MSG_TIMESTAMP;
    msg.timestamp_us = get_epoch_time_us();
    
    if (description != NULL) {
        strncpy(msg.description, description, sizeof(msg.description) - 1);
        msg.description[sizeof(msg.description) - 1] = '\0';
    } else {
        msg.description[0] = '\0';
    }
    
    if (LoRaSend((uint8_t*)&msg, sizeof(msg), SX126x_TXMODE_SYNC) == false) {
        ESP_LOGE(TAG, "Failed to send timestamp message");
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "Timestamp sent: %llu µs (%s) - %s", 
             msg.timestamp_us, formart_timestamp_us(msg.timestamp_us), 
             msg.description[0] ? msg.description : "no description");
    return ESP_OK;
}

// Task RX para MASTER (recebe requisições e envia respostas)
void lora_rx_task(void *pvParameters)
{
    ESP_LOGI(TAG, "LoRa receptor initiated.");
    uint8_t buf[256];
    
    while(1) {
        uint8_t rxLen = LoRaReceive(buf, sizeof(buf));
        if (rxLen > 0) {
            uint8_t msg_type = buf[0];
            
            if (msg_type == LORA_MSG_TIME_REQUEST && rxLen == sizeof(lora_time_sync_msg_t)) {
                lora_time_sync_msg_t *time_req = (lora_time_sync_msg_t*)buf;
                time_req->t2 = get_epoch_time_us(); // T2: momento da recepção (em µs)
                
                ESP_LOGI(TAG, "Time sync request received at T2=%llu µs (%s)", 
                         time_req->t2, formart_timestamp_us(time_req->t2));
                
                // Envia resposta imediatamente
                lora_send_time_response(time_req);
                
            } else if (msg_type == LORA_MSG_TIMESTAMP && rxLen == sizeof(lora_timestamp_msg_t)) {
                lora_timestamp_msg_t *msg = (lora_timestamp_msg_t*)buf;
                
                int8_t rssi, snr;
                GetPacketStatus(&rssi, &snr);
                
                ESP_LOGI(TAG, "Timestamp received: %llu µs (%s)", 
                         msg->timestamp_us, formart_timestamp_us(msg->timestamp_us));
                ESP_LOGI(TAG, "Description: %s", msg->description[0] ? msg->description : "none");
                ESP_LOGI(TAG, "RSSI=%d[dBm] SNR=%d[dB]", rssi, snr);

                char timestamp_str[32];
                snprintf(timestamp_str, sizeof(timestamp_str), "%llu", msg->timestamp_us);
                mqtt_publish_data(299, formart_timestamp_us(msg->timestamp_us));
            }
        }
        vTaskDelay(1);
    }
}

// Task TX para SLAVE (envia requisições e processa respostas)
void lora_tx_task(void *pvParameters)
{
    ESP_LOGI(TAG, "LoRa transmitter initiated.");
    uint8_t buf[256];
    bool time_synced = false;
    uint32_t msg_counter = 0;
    
    // Sincroniza tempo no início
    vTaskDelay(pdMS_TO_TICKS(2000));
    lora_request_timesync();
    
    uint32_t last_sync = xTaskGetTickCount();
    uint32_t last_msg = xTaskGetTickCount();
    
    while (1) {
        // Verifica se há mensagens recebidas
        uint8_t rxLen = LoRaReceive(buf, sizeof(buf));
        if (rxLen > 0) {
            uint8_t msg_type = buf[0];
            
            if (msg_type == LORA_MSG_TIME_RESPONSE && rxLen == sizeof(lora_time_sync_msg_t)) {
                lora_time_sync_msg_t *time_resp = (lora_time_sync_msg_t*)buf;
                process_time_response(time_resp);
                time_synced = true;
                ESP_LOGI(TAG, "Time synchronized! Ready to send messages.");
            }
        }
        
        // Se o tempo está sincronizado, envia mensagens a cada 5 segundos
        if (time_synced && (xTaskGetTickCount() - last_msg) > pdMS_TO_TICKS(5000)) {

            char desc[50];
            snprintf(desc, sizeof(desc), "Message #%lu", msg_counter);
            lora_send_timestamp(desc);
            
            msg_counter++;
            last_msg = xTaskGetTickCount();
        }
        
        // Re-sincroniza a cada 5 minutos
        if ((xTaskGetTickCount() - last_sync) > pdMS_TO_TICKS(150000)) {
            ESP_LOGI(TAG, "Re-synchronizing time...");
            lora_request_timesync();
            last_sync = xTaskGetTickCount();
            time_synced = false; // Aguarda confirmação de sincronização
        }
        
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

esp_err_t init_lora_slave()
{
    ESP_LOGI(TAG, "Initializing LoRa as slave...");
    
    // Brasil region (915 MHz)
    uint32_t frequencyInHz = 915000000; // 915 MHz for Brasil
    
    LoRaInit();
    
    int8_t txPowerInDbm = 22;        // Max pontency (22 dBm)
    float tcxoVoltage = 3.3;         // TCXO voltage
    bool useRegulatorLDO = true;     // Use LDO regulator
    
    if (LoRaBegin(frequencyInHz, txPowerInDbm, tcxoVoltage, useRegulatorLDO) != 0) {
        ESP_LOGE(TAG, "ERROR: LoRa module not recognized");
        return ESP_FAIL;
    }
    
    uint8_t spreadingFactor = 7;     // SF7 - balaced between range and data rate
    uint8_t bandwidth = 4;           // 125 kHz
    uint8_t codingRate = 1;          // 4/5
    uint16_t preambleLength = 8;     // Standard preamble length
    uint8_t payloadLen = 0;          // Variable Payload
    bool crcOn = true;               // CRC has enabled
    bool invertIrq = false;          // Normal IRQ
    
    LoRaConfig(spreadingFactor, bandwidth, codingRate, preambleLength, payloadLen, crcOn, invertIrq);
    
    ESP_LOGI(TAG, "Slave configured!");
    ESP_LOGI(TAG, "Frequency: %lu Hz", frequencyInHz);
    ESP_LOGI(TAG, "Signal strength: %d dBm", txPowerInDbm);
    ESP_LOGI(TAG, "SF: %d, BW: %d, CR: %d", spreadingFactor, bandwidth, codingRate);

    xTaskCreate(&lora_tx_task, "LORA_TX", 1024*4, NULL, 5, NULL);
    return ESP_OK;
}

esp_err_t init_lora_master()
{
    ESP_LOGI(TAG, "Initializing LoRa as master...");
    
    // Brasil region (915 MHz)
    uint32_t frequencyInHz = 915000000; // 915 MHz for Brasil
    
    LoRaInit();
    
    int8_t txPowerInDbm = 22;        // Max pontency (22 dBm)
    float tcxoVoltage = 3.3;         // TCXO voltage
    bool useRegulatorLDO = true;     // Use LDO regulator
    
    if (LoRaBegin(frequencyInHz, txPowerInDbm, tcxoVoltage, useRegulatorLDO) != 0) {
        ESP_LOGE(TAG, "ERROR: LoRa module not recognized");
        return ESP_FAIL;
    }
    
    uint8_t spreadingFactor = 7;     // SF7 - balaced between range and data rate
    uint8_t bandwidth = 4;           // 125 kHz
    uint8_t codingRate = 1;          // 4/5
    uint16_t preambleLength = 8;     // Standard preamble length
    uint8_t payloadLen = 0;          // Variable Payload
    bool crcOn = true;               // CRC has enabled
    bool invertIrq = false;          // Normal IRQ
    
    LoRaConfig(spreadingFactor, bandwidth, codingRate, preambleLength, payloadLen, crcOn, invertIrq);
    
    ESP_LOGI(TAG, "Master configured!");
    ESP_LOGI(TAG, "Frequency: %lu Hz", frequencyInHz);
    ESP_LOGI(TAG, "Signal strength: %d dBm", txPowerInDbm);
    ESP_LOGI(TAG, "SF: %d, BW: %d, CR: %d", spreadingFactor, bandwidth, codingRate);

    xTaskCreate(&lora_rx_task, "LORA_RX", 1024*4, NULL, 5, NULL);
    return ESP_OK;
}