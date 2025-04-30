#ifndef MQTT_ESP_H
#define MQTT_ESP_H


#include <mqtt_client.h>

esp_mqtt_client_handle_t mqtt_client();
void mqtt_publish(esp_mqtt_client_handle_t client, const char *data);
void mqtt_subscribe(esp_mqtt_client_handle_t client);

#endif
