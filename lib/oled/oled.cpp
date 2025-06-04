#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c.h"
#include "esp_log.h"
#include "ssd1306.h"
#include <string.h>

#define I2C_MASTER_SCL_IO           GPIO_NUM_18  // Pino SCL para o display embutido
#define I2C_MASTER_SDA_IO           GPIO_NUM_17  // Pino SDA para o display embutido
#define OLED_RESET_PIN              GPIO_NUM_21  // Pino RST opcional

#define I2C_MASTER_NUM              I2C_NUM_0
#define I2C_MASTER_FREQ_HZ          400000     
#define I2C_MASTER_TIMEOUT_MS       5000        

static const char *TAG = "OLED";

i2c_master_bus_config_t config_i2c_bus() {

    i2c_master_bus_config_t i2c_bus_config = {};
    i2c_bus_config.sda_io_num = I2C_MASTER_SDA_IO;
    i2c_bus_config.scl_io_num = I2C_MASTER_SCL_IO;
    i2c_bus_config.clk_source = I2C_CLK_SRC_DEFAULT;
    i2c_bus_config.glitch_ignore_cnt = 7;
    i2c_bus_config.flags.enable_internal_pullup = true;

    return i2c_bus_config;

}

i2c_device_config_t config_i2c_device() {

    i2c_device_config_t i2c_dev_config = {};
    i2c_dev_config.dev_addr_length = I2C_ADDR_BIT_LEN_7;
    i2c_dev_config.device_address = 0x3C;
    i2c_dev_config.scl_speed_hz = I2C_MASTER_FREQ_HZ;
    i2c_dev_config.scl_wait_us = 0;
    i2c_dev_config.flags.disable_ack_check = 0;

    return i2c_dev_config;

}


ssd1306_config_t ssd1306_config() {
    ssd1306_config_t config = {};
    config.panel_size = SSD1306_PANEL_128x64;
    config.i2c_address = 0x3C;  
    config.i2c_clock_speed = I2C_MASTER_FREQ_HZ;
    config.flip_enabled = false;
    config.display_enabled = true;
    return config;
}

esp_err_t init_oled(i2c_master_bus_handle_t i2c_bus_handle, ssd1306_config_t config, ssd1306_handle_t *ssd1306_handle) {
    gpio_reset_pin(OLED_RESET_PIN);
    gpio_set_direction(OLED_RESET_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(OLED_RESET_PIN, 0);
    gpio_set_level(OLED_RESET_PIN, 1);

    ESP_ERROR_CHECK(ssd1306_init(i2c_bus_handle, &config, ssd1306_handle));
    ESP_ERROR_CHECK(ssd1306_enable_display(*ssd1306_handle));
    ESP_ERROR_CHECK(ssd1306_clear_display(*ssd1306_handle, false));
    ESP_ERROR_CHECK(ssd1306_display_text(*ssd1306_handle, 1, "Initializing...", false));
    return ESP_OK;
}

void oled_print_message(ssd1306_handle_t ssd1306_handle, const char *message) {
    if (ssd1306_handle != NULL) {
        ESP_ERROR_CHECK_WITHOUT_ABORT(ssd1306_clear_display(ssd1306_handle, false));
        
        const int MAX_CHARS_PER_LINE = 16;
        const int MAX_LINES = 8;
        
        char buffer[256];
        strncpy(buffer, message, sizeof(buffer) - 1);
        buffer[sizeof(buffer) - 1] = '\0';
        
        char *line_start = buffer;
        int current_line = 0;
        
        while (line_start && *line_start && current_line < MAX_LINES) {
            current_line++;
            
            if (strlen(line_start) > MAX_CHARS_PER_LINE) {
                char temp = line_start[MAX_CHARS_PER_LINE];
                line_start[MAX_CHARS_PER_LINE] = '\0';
                
                ESP_ERROR_CHECK_WITHOUT_ABORT(ssd1306_display_text(ssd1306_handle, current_line, line_start, false));
                line_start[MAX_CHARS_PER_LINE] = temp;
                line_start += MAX_CHARS_PER_LINE;
            } else {
                ESP_ERROR_CHECK_WITHOUT_ABORT(ssd1306_display_text(ssd1306_handle, current_line, line_start, false));
                break;
            }
        }
    }
}