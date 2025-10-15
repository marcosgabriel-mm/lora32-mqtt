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
#include <mocks_esp.h>
#include <mqtt_esp.h>
#include <oled.h>

#include <mqtt_client.h>
#include <time_esp.h>
#include <cJSON.h>

extern "C" void app_main() {
    
    ESP_LOGI("MAIN", "Starting ESP32 application...");   
    
    i2c_master_bus_config_t i2c_bus_config = config_i2c_bus();
    i2c_master_bus_handle_t i2c_bus_handle = NULL;
    ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_bus_config, &i2c_bus_handle));

    i2c_device_config_t i2c_dev_conf = config_i2c_device();
    i2c_master_dev_handle_t i2c_dev_handle = NULL;
    ESP_ERROR_CHECK(i2c_master_bus_add_device(i2c_bus_handle, &i2c_dev_conf, &i2c_dev_handle));

    ssd1306_config_t config = ssd1306_config();
    ssd1306_handle_t ssd1306_handle = NULL;
    ESP_ERROR_CHECK(init_oled(i2c_bus_handle, config, &ssd1306_handle));
    
    oled_print_message(ssd1306_handle, "Configuring Device...");
    ESP_LOGI("MAIN", "OLED initialized successfully");

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    setup_eeprom();

    // reset_wifi_credentials(); // Forçar reconfiguração do WiFi a cada reinício (apenas para testes)
    
    if (!has_wifi_configured()) {
        start_wifi_ap();
        start_webserver();

        while (!wifi_credentials().connected) {
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            oled_print_message(ssd1306_handle, "Waiting for the user to configure the wifi...");
        }

        wifi_credentials_t credentials = wifi_credentials();
        ESP_LOGI("MAIN", "SSID: %s | Password: %s", credentials.ssid, credentials.password);
        
        save_wifi_credentials(credentials.ssid, credentials.password);
        set_wifi_configured(0);
    
        oled_print_message(ssd1306_handle, "Device configured successfully. Restarting...");
        esp_restart();
    }

    wifi_credentials_t credentials = load_wifi_credentials();
    oled_print_message(ssd1306_handle, "Connecting to WiFi and Syncing time...");
    ESP_ERROR_CHECK(wifi_init(credentials.ssid, credentials.password));
    ESP_ERROR_CHECK(sync_time());

    size_t free_heap_size = esp_get_free_heap_size();
    ESP_LOGI("MAIN", "Free heap size: %d bytes | %d megabytes", free_heap_size, free_heap_size / 1024);

    mqtt_task_params_t* params = (mqtt_task_params_t*) malloc(sizeof(mqtt_task_params_t));
    params->client = mqtt_client();
    oled_print_message(ssd1306_handle, "Connecting to MQTT broker...");
    oled_print_message(ssd1306_handle, "All systems operational");

    xTaskCreate(mqtt_publish, "mqtt_publish", 4096, (void*) params, 5, NULL);

    while(1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}