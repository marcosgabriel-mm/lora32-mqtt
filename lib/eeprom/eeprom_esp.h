#ifndef EEPROM_ESP_H
#define EEPROM_ESP_H

#include <wifi_credentials_page/wifi_credentials.h>

#ifdef __cplusplus
extern "C" {
#endif

void setup_eeprom(void);
bool has_wifi_configured(void);
void reset_wifi_credentials(void);
void set_wifi_configured(uint8_t has_data);
void save_wifi_credentials(const char *ssid, const char *password);
wifi_credentials_t load_wifi_credentials(void);

#ifdef __cplusplus
}
#endif

#endif // EEPROM_ESP_H