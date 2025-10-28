#define ESP_NOW_LOCAL_H
#ifdef ESP_NOW_LOCAL_H

typedef struct {
    int id;
    char message[250];
} struct_message_t;

esp_err_t espnow_init(void);
esp_err_t espnow_slave_init(uint8_t *master_mac);

#endif