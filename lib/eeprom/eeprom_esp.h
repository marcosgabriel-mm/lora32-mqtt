#define EEPROM_ESP_H
#ifdef EEPROM_ESP_H

#include <wifi_credentials_page/wifi_credentials.h>

void setup_eeprom();
bool has_wifi_configured();
void reset_wifi_credentials();
void set_wifi_configured(uint8_t has_data);
void save_wifi_credentials(const char *ssid, const char *password);
wifi_credentials_t load_wifi_credentials();

#endif