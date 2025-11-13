#include "ra01s.h"
#include <esp_log.h>

static const char* TAG = "LoRa";

void lora_rx_task(void *pvParameters)
{
    ESP_LOGI(TAG, "LoRa Receptor iniciado");
    uint8_t buf[256]; // Tamanho máximo do payload SX1261/62/68 é 255
    
    while(1) {
        uint8_t rxLen = LoRaReceive(buf, sizeof(buf));
        if (rxLen > 0) { 
            // Adicionar terminador de string para segurança
            buf[rxLen] = '\0';
            
            ESP_LOGI(TAG, "Recebido %d bytes: [%.*s]", rxLen, rxLen, buf);
            
            // Obter informações do sinal
            int8_t rssi, snr;
            GetPacketStatus(&rssi, &snr);
            ESP_LOGI(TAG, "RSSI=%d[dBm] SNR=%d[dB]", rssi, snr);
            
            // Aqui você pode processar a mensagem e enviar via MQTT
            // Exemplo: mqtt_publish("lora/data", (char*)buf);
        }
        vTaskDelay(1); // Evitar alertas do WatchDog
    }
}

esp_err_t init_lora_master()
{
    ESP_LOGI(TAG, "Inicializando LoRa como MESTRE...");
    
    // Inicializar Arduino (Verificar se necessário)
    // initArduino();
    
    // Configurações para região brasileira (915 MHz)
    uint32_t frequencyInHz = 915000000; // 915 MHz para Brasil
    ESP_LOGI(TAG, "Frequência configurada: %lu Hz", frequencyInHz);
    
    // Inicializar LoRa
    LoRaInit();
    
    // Configurações de potência e voltagem para Heltec V3
    int8_t txPowerInDbm = 22;        // Potência máxima (22 dBm)
    float tcxoVoltage = 3.3;         // Voltagem do cristal
    bool useRegulatorLDO = true;     // Usar regulador LDO
    
    if (LoRaBegin(frequencyInHz, txPowerInDbm, tcxoVoltage, useRegulatorLDO) != 0) {
        ESP_LOGE(TAG, "ERRO: Módulo LoRa não reconhecido");
        return ESP_FAIL;
    }
    
    // Configurações LoRa otimizadas para longo alcance
    uint8_t spreadingFactor = 7;     // SF7 - balanceamento entre velocidade e alcance
    uint8_t bandwidth = 4;           // 125 kHz
    uint8_t codingRate = 1;          // 4/5
    uint16_t preambleLength = 8;     // Preâmbulo padrão
    uint8_t payloadLen = 0;          // Payload variável
    bool crcOn = true;               // Habilitar CRC
    bool invertIrq = false;          // IRQ normal
    
    LoRaConfig(spreadingFactor, bandwidth, codingRate, preambleLength, payloadLen, crcOn, invertIrq);
    
    ESP_LOGI(TAG, "=== MESTRE LORA CONFIGURADO ===");
    ESP_LOGI(TAG, "Frequência: %lu Hz", frequencyInHz);
    ESP_LOGI(TAG, "Potência: %d dBm", txPowerInDbm);
    ESP_LOGI(TAG, "SF: %d, BW: %d, CR: %d", spreadingFactor, bandwidth, codingRate);
    
    // Criar task para recepção LoRa
    xTaskCreate(&lora_rx_task, "LORA_RX", 1024*4, NULL, 5, NULL);
    
    return ESP_OK;
}