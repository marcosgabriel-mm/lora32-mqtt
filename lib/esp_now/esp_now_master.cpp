#include "esp_now.h"
#include "esp_now_local.h"

#include <esp_wifi.h>
#include <esp_log.h>
#include <string.h>

#include <mqtt_esp.h>
#include <mqtt_client.h>

void master_recv_cb(const esp_now_recv_info_t *esp_now_info, const uint8_t *data, int data_len) {
    
    struct_message_t receivedData;
    memcpy(&receivedData, data, sizeof(receivedData));
    
    ESP_LOGI("ESP-NOW", "Package received from slave ID: %d", receivedData.id);
    ESP_LOGI("ESP-NOW", "Data: %s", receivedData.message);

    // mqtt_publish_data(receivedData.id, receivedData.message);
}

esp_err_t espnow_init(void) {
 
    uint8_t primary;
    wifi_second_chan_t second;
    ESP_ERROR_CHECK(esp_wifi_get_channel(&primary, &second));
    ESP_LOGI("ESP-NOW", "WiFi channel: %d", primary);

    if (esp_now_init() != ESP_OK) {
        ESP_LOGE("ESP-NOW", "Error on initialize ESP-NOW");
        return ESP_FAIL;
    }

    ESP_ERROR_CHECK(esp_now_register_recv_cb(master_recv_cb));
    ESP_LOGI("ESP-NOW", "Master ready to receive data on channel %d", primary);

    return ESP_OK;
}