#include "esp_now.h"
#include "esp_now_local.h"
#include <esp_wifi.h>
#include <esp_log.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "time_esp.h"

// Variável global para armazenar o MAC do mestre
static uint8_t stored_master_mac[6];
static esp_time_sync_t time_sync_info = {0};

void slave_send_cb(const uint8_t *mac_addr, esp_now_send_status_t status) {
    if (status == ESP_NOW_SEND_SUCCESS) {
        ESP_LOGI("ESP-NOW", "Message sent successfully to master");
    } else {
        ESP_LOGE("ESP-NOW", "Failed to send message to master");
    }
}

void slave_recv_cb(const esp_now_recv_info_t *esp_now_info, const uint8_t *data, int data_len) {
    struct_message_t receivedData;
    memcpy(&receivedData, data, sizeof(receivedData));
    
    if (receivedData.type == MSG_TYPE_TIME_RESPONSE) {
        int64_t current_time = get_epoch_time_us();
        int64_t master_time = receivedData.timestamp_us;
        
        // Calcular offset e delay da rede (estimativa simples)
        time_sync_info.time_offset_us = master_time - current_time;
        time_sync_info.network_delay_us = (current_time - time_sync_info.last_sync_time) / 2;
        time_sync_info.last_sync_time = current_time;
        time_sync_info.is_synchronized = true;
        time_sync_info.sync_quality = 85; // Qualidade estimada
        
        ESP_LOGI("ESP-NOW", "Time synchronized! Offset: %lld us", time_sync_info.time_offset_us);
        ESP_LOGI("ESP-NOW", "Master time: %lld, Local time: %lld", master_time, current_time);
    }
}

esp_err_t espnow_send_to_master(uint8_t *master_mac, struct_message_t *message) {
    esp_err_t result = esp_now_send(master_mac, (uint8_t *)message, sizeof(struct_message_t));
    
    if (result == ESP_OK) {
        ESP_LOGI("ESP-NOW", "Sending message to master...");
    } else {
        ESP_LOGE("ESP-NOW", "Error sending message: %s", esp_err_to_name(result));
    }
    
    return result;
}

void slave_send_task(void *pvParameters) {
    struct_message_t message;
    message.type = MSG_TYPE_DATA;
    message.id = 1;
    int counter = 0;
    
    while(1) {
        snprintf(message.message, sizeof(message.message), "Data packet %d", counter);
        message.timestamp_us = get_synchronized_time();
        
        ESP_LOGI("ESP-NOW", "Time formated: %s", get_format_time());
        ESP_LOGI("ESP-NOW", "Sending: %s", message.message);
        
        esp_err_t result = espnow_send_to_master(stored_master_mac, &message);
        if (result != ESP_OK) {
            ESP_LOGE("ESP-NOW", "Failed to send data to master");
        }
        
        if (counter % 50 == 0) {
            request_time_sync(stored_master_mac);
        }

        counter++;
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

bool test_master_connection(uint8_t *master_mac) {
    struct_message_t test_message;
    test_message.type = MSG_TYPE_DATA;
    test_message.id = 999;
    strcpy(test_message.message, "Connection test");
    test_message.timestamp_us = 0;
    
    esp_err_t result = esp_now_send(master_mac, (uint8_t *)&test_message, sizeof(test_message));
    return (result == ESP_OK);
}

esp_err_t request_time_sync(uint8_t *master_mac) {
    struct_message_t time_request;
    time_request.type = MSG_TYPE_TIME_REQUEST;
    time_request.id = 1; // Slave ID
    time_request.timestamp_us = get_epoch_time_us();
    
    
    strcpy(time_request.message, "Time sync request");
    time_sync_info.last_sync_time = time_request.timestamp_us;
    
    esp_err_t result = esp_now_send(master_mac, (uint8_t *)&time_request, sizeof(time_request));
    
    if (result == ESP_OK) {
        ESP_LOGI("ESP-NOW", "Time sync request sent to master");
    } else {
        ESP_LOGE("ESP-NOW", "Failed to send time sync request: %s", esp_err_to_name(result));
    }
    
    return result;
}

int64_t get_synchronized_time() {
    if (time_sync_info.is_synchronized) {
        return get_epoch_time_us() + time_sync_info.time_offset_us;
    } else {
        ESP_LOGW("ESP-NOW", "Time not synchronized, returning local time");
        return get_epoch_time_us();
    }
}

esp_err_t espnow_slave_init(uint8_t *master_mac) {
    ESP_ERROR_CHECK(esp_wifi_set_channel(6, WIFI_SECOND_CHAN_NONE));
    ESP_LOGI("ESP-NOW", "WiFi channel set to 6");
    
    vTaskDelay(pdMS_TO_TICKS(100));
    
    if (esp_now_init() != ESP_OK) {
        ESP_LOGE("ESP-NOW", "Error initializing ESP-NOW");
        return ESP_FAIL;
    }

    memcpy(stored_master_mac, master_mac, 6);

    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, master_mac, 6);
    peerInfo.channel = 6;
    peerInfo.encrypt = false;

    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
        ESP_LOGE("ESP-NOW", "Failed to add peer");
        return ESP_FAIL;
    }

    ESP_ERROR_CHECK(esp_now_register_recv_cb(slave_recv_cb));
    ESP_ERROR_CHECK(esp_now_register_send_cb(slave_send_cb));
    ESP_LOGI("ESP-NOW", "ESP-NOW slave initialized successfully");

    ESP_LOGI("ESP-NOW", "Testing master connection...");
    int max_attempts = 30;
    int attempt = 0;
    
    while (attempt < max_attempts) {
        if (test_master_connection(master_mac)) {
            ESP_LOGI("ESP-NOW", "Master is ready! Starting communication...");
            break;
        }
        
        ESP_LOGI("ESP-NOW", "Master not ready yet, attempt %d/%d", attempt + 1, max_attempts);
        vTaskDelay(pdMS_TO_TICKS(1000));
        attempt++;
    }
    
    if (attempt >= max_attempts) {
        ESP_LOGW("ESP-NOW", "Master may not be ready, but starting task anyway...");
    }

    request_time_sync(master_mac);
    BaseType_t task_result = xTaskCreate(slave_send_task, "slave_send_task", 4096, NULL, 5, NULL);
    if (task_result == pdPASS) {
        ESP_LOGI("ESP-NOW", "Slave send task created successfully");
    } else {
        ESP_LOGE("ESP-NOW", "Failed to create slave send task");
        return ESP_FAIL;
    }

    return ESP_OK;
}