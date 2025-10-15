#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_netif.h"

#include <wifi_esp.h>
#include <eeprom_esp.h>
#include <wifi_credentials_page/wifi_credentials.h>
#include <mqtt_esp.h>

#include <mqtt_client.h>
#include <time_esp.h>
#include <cJSON.h>

extern "C" void app_main() {
    
    ESP_LOGI("MAIN", "Starting ESP32 application...");   

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    setup_eeprom();

    // reset_wifi_credentials(); // Forçar reconfiguração do WiFi a cada reinício (apenas para testes)
    
    if (!has_wifi_configured()) {
        start_wifi_ap();
        start_webserver();

        while (!wifi_credentials().connected) {
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }

        wifi_credentials_t credentials = wifi_credentials();
        ESP_LOGI("MAIN", "SSID: %s | Password: %s", credentials.ssid, credentials.password);
        
        save_wifi_credentials(credentials.ssid, credentials.password);
        set_wifi_configured(0);
    
        esp_restart();
    }

    wifi_credentials_t credentials = load_wifi_credentials();
    ESP_ERROR_CHECK(wifi_init(credentials.ssid, credentials.password));
    ESP_ERROR_CHECK(sync_time());

    size_t free_heap_size = esp_get_free_heap_size();
    ESP_LOGI("MAIN", "Free heap size: %d bytes | %d megabytes", free_heap_size, free_heap_size / 1024);

    mqtt_task_params_t* params = (mqtt_task_params_t*) malloc(sizeof(mqtt_task_params_t));
    params->client = mqtt_client();

    xTaskCreate(mqtt_publish, "mqtt_publish", 4096, (void*) params, 5, NULL);

    while(1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}