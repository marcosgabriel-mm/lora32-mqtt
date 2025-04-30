#include "cJSON.h"
#include "time_esp.h"
#include <esp_log.h>

cJSON *time_data_mock_json() {

    cJSON *json = cJSON_CreateObject();
    if (json == NULL) {
        ESP_LOGE("TIME", "Failed to create JSON object");
        return NULL;
    }

    char *date = get_format_time();
    if (date == NULL) {
        ESP_LOGE("TIME", "Failed to get formatted time");
        cJSON_Delete(json);
        return NULL;
    }

    cJSON_AddStringToObject(json, "date", date);
    cJSON_AddStringToObject(json, "timezone", "UTC");

    return json;
}
