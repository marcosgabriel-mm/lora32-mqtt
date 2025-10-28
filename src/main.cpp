#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_netif.h"
#include "esp_now_local.h"

#include <wifi_esp.h>
#include <esp_wifi.h>
#include <eeprom_esp.h>
#include <wifi_credentials_page/wifi_credentials.h>
#include <mqtt_esp.h>

#include <mqtt_client.h>
#include <time_esp.h>
#include <cJSON.h>
#include <esp_mac.h>

#define IS_MASTER false
uint8_t master_mac[6] = {0xCC, 0xDB, 0xA7, 0xFA, 0xED, 0x1C}; // Substitua pelo MAC real do mestre

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

        uint8_t mac[6];
        esp_read_mac(mac, ESP_MAC_BASE);
        ESP_LOGI("MAIN", "AP MAC Address: %02X:%02X:%02X:%02X:%02X:%02X", 
            mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

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
        
        uint8_t real_mac[6];
        esp_read_mac(real_mac, ESP_MAC_WIFI_STA);
        ESP_LOGI("MASTER", "My real STA MAC: %02X:%02X:%02X:%02X:%02X:%02X", 
            real_mac[0], real_mac[1], real_mac[2], real_mac[3], real_mac[4], real_mac[5]);

        wifi_credentials_t credentials = load_wifi_credentials();
        ESP_ERROR_CHECK(wifi_init(credentials.ssid, credentials.password));
        ESP_ERROR_CHECK(sync_time());

        // ESP_ERROR_CHECK(mqtt_init());
        ESP_ERROR_CHECK(espnow_init());
        ESP_LOGI("MASTER", "Master initialized - waiting for slaves...");
    
    } else {
        ESP_LOGI("MAIN", "Starting as SLAVE");
        
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
        ESP_ERROR_CHECK(esp_wifi_start());

        ESP_LOGI("SLAVE", "Target master MAC: %02x:%02x:%02x:%02x:%02x:%02x", 
            master_mac[0], master_mac[1], master_mac[2], 
            master_mac[3], master_mac[4], master_mac[5]);

        ESP_ERROR_CHECK(espnow_slave_init(master_mac));
    }

    size_t free_heap_size = esp_get_free_heap_size();
    ESP_LOGI("MAIN", "Free heap size: %d bytes | %d KB", free_heap_size, free_heap_size / 1024);

    while(1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}