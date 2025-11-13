#include "ra01s.h"
#include <esp_log.h>
#include <mqtt_esp.h>

static const char* TAG = "LoRa";

void lora_rx_task(void *pvParameters)
{
    ESP_LOGI(TAG, "LoRa receptor initiaded.");
    uint8_t buf[256]; // Max payload length SX1261/62/68 is 255
    
    while(1) {
        uint8_t rxLen = LoRaReceive(buf, sizeof(buf));
        if (rxLen > 0) { 
            buf[rxLen] = '\0';            
            ESP_LOGI(TAG, "Recived %d bytes: [%.*s]", rxLen, rxLen, buf);
            
            int8_t rssi, snr;
            GetPacketStatus(&rssi, &snr);
            ESP_LOGI(TAG, "RSSI=%d[dBm] SNR=%d[dB]", rssi, snr);
            
            // Process the received data (for example, publish to MQTT)
            mqtt_publish_data(299, "Teste de LoRa");
        }
        vTaskDelay(1);
    }
}

esp_err_t init_lora_master()
{
    ESP_LOGI(TAG, "Initializing LoRa as master...");
    
    // Brasil region (915 MHz)
    uint32_t frequencyInHz = 915000000; // 915 MHz for Brasil
    ESP_LOGI(TAG, "Frequency: %lu Hz", frequencyInHz);
    
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
    bool invertIrq = false;          // Norma IRQ
    
    LoRaConfig(spreadingFactor, bandwidth, codingRate, preambleLength, payloadLen, crcOn, invertIrq);
    
    ESP_LOGI(TAG, "Master configured!");
    ESP_LOGI(TAG, "Frequency: %lu Hz", frequencyInHz);
    ESP_LOGI(TAG, "Signal strength: %d dBm", txPowerInDbm);
    ESP_LOGI(TAG, "SF: %d, BW: %d, CR: %d", spreadingFactor, bandwidth, codingRate);
    
    xTaskCreate(&lora_rx_task, "LORA_RX", 1024*4, NULL, 5, NULL);
    return ESP_OK;
}