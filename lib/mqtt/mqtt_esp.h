#ifndef MQTT_ESP_H
#define MQTT_ESP_H

#include <mqtt_client.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    esp_mqtt_client_handle_t client;
} mqtt_task_params_t;

esp_mqtt_client_handle_t mqtt_client(void);
void mqtt_publish(void* pvParameters);
void mqtt_subscribe(esp_mqtt_client_handle_t client);

#ifdef __cplusplus
}
#endif

#endif // MQTT_ESP_H
