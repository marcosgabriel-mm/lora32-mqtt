#include "cJSON.h"
#include "time_esp.h"
#include <esp_log.h>

cJSON *time_data_mock_json() {
    cJSON *json = cJSON_CreateObject();
    if (json == NULL) {
        ESP_LOGE("TIME", "Failed to create JSON object");
        return NULL;
    }

    // Obter os dados de tempo em diferentes formatos
    time_t epoch_seconds = get_epoch_time();
    int64_t epoch_ms = get_epoch_time_ms();
    
    // Formatação de data e hora local com milissegundos
    struct tm timeinfo;
    localtime_r(&epoch_seconds, &timeinfo);
    int ms_part = epoch_ms % 1000;
    
    char formatted_date[64];
    snprintf(formatted_date, sizeof(formatted_date), 
            "%02d/%02d/%04d %02d:%02d:%02d.%03d",
            timeinfo.tm_mday, timeinfo.tm_mon + 1, timeinfo.tm_year + 1900,
            timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec, ms_part);
    
    // Adicionar ao JSON todos os formatos (usando tipos corretos)
    char epoch_seconds_str[16];
    sprintf(epoch_seconds_str, "%ld", (long)epoch_seconds);
    
    char epoch_ms_str[20];
    sprintf(epoch_ms_str, "%lld", epoch_ms);
    
    cJSON_AddStringToObject(json, "epoch_seconds", epoch_seconds_str);
    cJSON_AddStringToObject(json, "epoch_ms", epoch_ms_str);
    cJSON_AddStringToObject(json, "formatted_date", formatted_date);

    return json;
}
