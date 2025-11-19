#include <mqtt_client.h>
#include <esp_log.h>
#include "mqtt_esp.h"
#include <time_esp.h>
#include <cJSON.h>

#define MQTT_BROKER_URI "mqtt://10.5.223.152:1883"
#define MQTT_TOPIC "mqtt-lora32"

static esp_mqtt_client_handle_t global_client = NULL;

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

esp_err_t mqtt_init() {
    if (global_client != NULL) {
        return ESP_OK;
    }

    esp_mqtt_client_config_t mqtt_cfg = config_mqtt();
    global_client = esp_mqtt_client_init(&mqtt_cfg);
    
    if (global_client == NULL) {
        ESP_LOGE("MQTT", "Failed to create MQTT client");
        return ESP_FAIL;
    }

    esp_err_t err = esp_mqtt_client_start(global_client);
    if (err != ESP_OK) {
        ESP_LOGE("MQTT", "Failed to start MQTT client");
        return ESP_FAIL;
    }

    ESP_LOGI("MQTT", "MQTT client initialized");
    return ESP_OK;
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


void mqtt_publish_data(int slave_id, const char* message_data) {
    esp_mqtt_client_handle_t client = mqtt_client();
    
    if (client == NULL) {
        ESP_LOGE("MQTT", "MQTT client not initialized");
        return;
    }

    cJSON *json = cJSON_CreateObject();
    
    cJSON *slave_id_json = cJSON_CreateNumber(slave_id);
    cJSON *message = cJSON_CreateString(message_data);
    cJSON *timestamp = cJSON_CreateString(formart_timestamp_us(get_epoch_time_us()));
    
    cJSON_AddItemToObject(json, "slave_id", slave_id_json);
    cJSON_AddItemToObject(json, "slave_time", message);
    cJSON_AddItemToObject(json, "master_time", timestamp);
    
    char *json_string = cJSON_Print(json);
    int msg_id = esp_mqtt_client_publish(client, MQTT_TOPIC, json_string, 0, 1, 0);
    
    if (msg_id == -1) {
        ESP_LOGE("MQTT", "Failed to publish message");
    } else {
        ESP_LOGI("MQTT", "Message published with ID: %d", msg_id);
        ESP_LOGI("MQTT", "Published: %s", json_string);
    }

    free(json_string);
    cJSON_Delete(json);
}

esp_mqtt_client_handle_t mqtt_client() {
    return global_client;
}