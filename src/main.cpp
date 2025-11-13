#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_netif.h"
#include "ra01s.h"

#include <wifi_esp.h>
#include <esp_wifi.h>
#include <eeprom_esp.h>
#include <wifi_credentials_page/wifi_credentials.h>
#include <mqtt_esp.h>

#include <mqtt_client.h>
#include <lora_esp.h>
#include <time_esp.h>
#include <cJSON.h>
#include <esp_mac.h>

#define IS_MASTER true

extern "C" void app_main() {
    
    ESP_LOGI("MAIN", "Starting ESP32 application...");   

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    setup_eeprom();
    
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    if (!has_wifi_configured() && IS_MASTER) {
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

    if (IS_MASTER) {
        ESP_LOGI("MAIN", "Starting as LoRa master device...");

        wifi_credentials_t credentials = load_wifi_credentials();
        ESP_ERROR_CHECK(wifi_init(credentials.ssid, credentials.password));
        ESP_ERROR_CHECK(sync_time());

        //! ESP_ERROR_CHECK(mqtt_init());
        // Implement LoRa | SX1262;
        // On receiving data, publish via MQTT
    
    } else {
        ESP_LOGI("MAIN", "Starting as LoRa slave device...");
        
    }

    size_t free_heap_size = esp_get_free_heap_size();
    ESP_LOGI("MAIN", "Free heap size: %d bytes | %d KB", free_heap_size, free_heap_size / 1024);

    while(1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}