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

void slave_send_cb(const uint8_t *mac_addr, esp_now_send_status_t status) {
    if (status == ESP_NOW_SEND_SUCCESS) {
        ESP_LOGI("ESP-NOW", "Message sent successfully to master");
    } else {
        ESP_LOGE("ESP-NOW", "Failed to send message to master");
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
    message.id = 1;
    int counter = 0;
    
    while(1) {
        
        snprintf(message.message, sizeof(message.message), 
                "Slave data #%d - Time: %lld", counter++, get_epoch_time());
        
        ESP_LOGI("ESP-NOW", "Sending: %s", message.message);
        
        esp_err_t result = espnow_send_to_master(stored_master_mac, &message);
        if (result != ESP_OK) {
            ESP_LOGE("ESP-NOW", "Failed to send data to master");
        }
        
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

bool test_master_connection(uint8_t *master_mac) {
    struct_message_t test_message;
    test_message.id = 999;
    strcpy(test_message.message, "Connection test");
    
    esp_err_t result = esp_now_send(master_mac, (uint8_t *)&test_message, sizeof(test_message));
    return (result == ESP_OK);
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

    BaseType_t task_result = xTaskCreate(slave_send_task, "slave_send_task", 4096, NULL, 5, NULL);
    if (task_result == pdPASS) {
        ESP_LOGI("ESP-NOW", "Slave send task created successfully");
    } else {
        ESP_LOGE("ESP-NOW", "Failed to create slave send task");
        return ESP_FAIL;
    }

    return ESP_OK;
}