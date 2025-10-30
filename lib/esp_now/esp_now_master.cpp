#include "esp_now.h"
#include "esp_now_local.h"
#include "time_esp.h"

#include <esp_wifi.h>
#include <esp_log.h>
#include <string.h>

#include <mqtt_esp.h>
#include <mqtt_client.h>

esp_err_t add_peer_if_not_exists(const uint8_t *peer_addr) {
    if (!esp_now_is_peer_exist(peer_addr)) {
        esp_now_peer_info_t peerInfo = {};
        memcpy(peerInfo.peer_addr, peer_addr, 6);
        peerInfo.channel = 6;
        peerInfo.encrypt = false;
        
        esp_err_t result = esp_now_add_peer(&peerInfo);
        if (result == ESP_OK) {
            ESP_LOGI("ESP-NOW", "Peer added successfully");
        } else {
            ESP_LOGE("ESP-NOW", "Failed to add peer: %s", esp_err_to_name(result));
        }
        return result;
    }
    return ESP_OK;
}

void send_time_response(const uint8_t *slave_mac, int64_t request_time) {
    if (add_peer_if_not_exists(slave_mac) != ESP_OK) {
        ESP_LOGE("ESP-NOW", "Failed to add slave as peer");
        return;
    }

    struct_message_t response;
    response.type = MSG_TYPE_TIME_RESPONSE;
    response.id = 0; // Master ID
    response.timestamp_us = get_epoch_time_us();
    
    snprintf(response.message, sizeof(response.message), 
             "Time response - Master time: %lld", response.timestamp_us);
    
    esp_err_t result = esp_now_send(slave_mac, (uint8_t *)&response, sizeof(response));
    if (result == ESP_OK) {
        ESP_LOGI("ESP-NOW", "Time response sent to slave");
    } else {
        ESP_LOGE("ESP-NOW", "Failed to send time response: %s", esp_err_to_name(result));
    }
}

void master_recv_cb(const esp_now_recv_info_t *esp_now_info, const uint8_t *data, int data_len) {
    
    struct_message_t receivedData;
    memcpy(&receivedData, data, sizeof(receivedData));
    
    if (receivedData.type > MSG_TYPE_TIME_RESPONSE) {
        ESP_LOGW("ESP-NOW", "Invalid message type received: %d", receivedData.type);
        return;
    }
    
    switch (receivedData.type) {
        case MSG_TYPE_DATA:
            ESP_LOGI("ESP-NOW", "Package received from slave ID: %d", receivedData.id);
            
            ESP_LOGI("ESP-NOW", "Data: %s", receivedData.message);
            ESP_LOGI("ESP-NOW", "Timestamp received: %lld", receivedData.timestamp_us);
            ESP_LOGI("ESP-NOW", "Actual Timestamp: %lld", get_epoch_time_us());

            ESP_LOGI("ESP-NOW", "Formatted time: %s", formart_timestamp_us(receivedData.timestamp_us));
            ESP_LOGI("ESP-NOW", "Current time: %s", get_format_time());
            //! mqtt_publish_data(receivedData.id, receivedData.message);
            break;
            
        case MSG_TYPE_TIME_REQUEST:
            ESP_LOGI("ESP-NOW", "Time request received from slave ID: %d", receivedData.id);
            send_time_response(esp_now_info->src_addr, receivedData.timestamp_us);
            break;
            
        default:
            ESP_LOGW("ESP-NOW", "Unknown message type received: %d", receivedData.type);
            break;
    }
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