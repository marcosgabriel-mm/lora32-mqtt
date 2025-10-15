#include <mqtt_client.h>
#include <esp_log.h>
#include "mqtt_esp.h"
#include <time_esp.h>

#define MQTT_BROKER_URI "mqtt://10.5.223.152:1883"
#define MQTT_TOPIC "mqtt-lora32"

esp_mqtt_client_config_t config_mqtt() {
    esp_mqtt_client_config_t mqtt_cfg = {};
    mqtt_cfg.broker.address.uri = MQTT_BROKER_URI;
    mqtt_cfg.broker.verification.skip_cert_common_name_check = true;
    mqtt_cfg.network.disable_auto_reconnect = false;
    mqtt_cfg.network.timeout_ms = 15000;
    mqtt_cfg.task.priority = 5;
    mqtt_cfg.session.keepalive = 60;

    return mqtt_cfg;
}

esp_mqtt_client_handle_t mqtt_client() {
    
    esp_mqtt_client_config_t mqtt_cfg = config_mqtt();
    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    if (client == NULL) {
        ESP_LOGE("MQTT", "Failed to create MQTT client");
        return NULL;
    }

    esp_err_t err = esp_mqtt_client_start(client);
    if (err != ESP_OK) {
        ESP_LOGE("MQTT", "Failed to start MQTT client: %s", esp_err_to_name(err));
        return NULL;
    }

    return client;
}

void mqtt_subscribe(esp_mqtt_client_handle_t client) {
    if (client == NULL) {
        ESP_LOGE("MQTT", "Client is NULL");
        return;
    }

    int msg_id = esp_mqtt_client_subscribe(client, MQTT_TOPIC, 0);
    if (msg_id == -1) {
        ESP_LOGE("MQTT", "Failed to subscribe to topic");
    } else {
        ESP_LOGI("MQTT", "Subscribed to topic with ID: %d", msg_id);
    }
}

void mqtt_publish(void* pvParameters) {
    mqtt_task_params_t* params = (mqtt_task_params_t*) pvParameters;
    
    while(1) {
        const char* epoch_time_str = get_format_time();
        
        int msg_id = esp_mqtt_client_publish(params->client, MQTT_TOPIC, epoch_time_str, 0, 1, 0);
        if (msg_id == -1) {
            ESP_LOGE("MQTT", "Failed to publish message");
        } else {
            ESP_LOGI("MQTT", "Message published with ID: %d", msg_id);
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    free(params);
    vTaskDelete(NULL);
}