#ifndef OLED_H
#define OLED_H

#include <driver/i2c_types.h>
#include <driver/i2c_master.h>
#include <ssd1306.h>

#ifdef __cplusplus
extern "C" {
#endif

void oled_print_message(ssd1306_handle_t ssd1306_handle, const char *message);
esp_err_t init_oled(i2c_master_bus_handle_t i2c_bus_handle, ssd1306_config_t config, ssd1306_handle_t *ssd1306_handle);
i2c_master_bus_config_t config_i2c_bus(void);
i2c_device_config_t config_i2c_device(void);
ssd1306_config_t ssd1306_config(void);

#ifdef __cplusplus
}
#endif

#endif // OLED_H
