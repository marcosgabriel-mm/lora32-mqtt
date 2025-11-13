#define OLED_H
#ifdef OLED_H

#include <driver/i2c_types.h>
#include <driver/i2c_master.h>
#include <ssd1306.h>

void oled_print_message(ssd1306_handle_t ssd1306_handle, const char *message);
esp_err_t init_oled(i2c_master_bus_handle_t i2c_bus_handle, ssd1306_config_t config, ssd1306_handle_t *ssd1306_handle);
i2c_master_bus_config_t config_i2c_bus();
i2c_device_config_t config_i2c_device();
ssd1306_config_t ssd1306_config();

#endif