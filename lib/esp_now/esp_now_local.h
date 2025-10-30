#define ESP_NOW_LOCAL_H
#ifdef ESP_NOW_LOCAL_H

typedef enum {
    MSG_TYPE_DATA = 0,
    MSG_TYPE_TIME_REQUEST = 1,
    MSG_TYPE_TIME_RESPONSE = 2
} message_type_t;

typedef struct {
    message_type_t type;
    int id;
    char message[250];
    int64_t timestamp_us;
} struct_message_t;

typedef struct {
    int64_t time_offset_us;     // Offset em microsegundos
    int64_t network_delay_us;   // Delay de rede estimado
    int64_t last_sync_time;     // Timestamp da última sincronização
    bool is_synchronized;       // Status da sincronização
    int sync_quality;           // Qualidade da sincronização (0-100)
} esp_time_sync_t;

esp_err_t espnow_init(void);
esp_err_t espnow_slave_init(uint8_t *master_mac);
esp_err_t request_time_sync(uint8_t *master_mac);
int64_t get_synchronized_time(void);

#endif