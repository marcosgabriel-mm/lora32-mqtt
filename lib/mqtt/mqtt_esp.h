#ifndef MQTT_ESP_H
#define MQTT_ESP_H

#include <mqtt_client.h>

esp_err_t mqtt_init();
esp_mqtt_client_handle_t mqtt_client();

void mqtt_publish_data(int slave_id, const char* message_data);
void mqtt_subscribe(esp_mqtt_client_handle_t client);

#endif