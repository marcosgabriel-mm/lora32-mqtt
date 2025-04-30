#include <nvs_flash.h>
#include <nvs.h>
#include <esp_log.h>
#include <wifi_credentials_page/wifi_credentials.h>

void setup_eeprom() {
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
}

nvs_handle_t eeprom_handler() {
    nvs_handle_t my_handle;
    esp_err_t err;

    err = nvs_open("storage", NVS_READWRITE, &my_handle);
    if (err != ESP_OK) {
        ESP_LOGE("EEPROM", "Error (%s) opening NVS handle!\n", esp_err_to_name(err));
    } 

    return my_handle;
}

bool has_wifi_configured() {
    nvs_handle_t handler = eeprom_handler();
    uint8_t has_data;

    esp_err_t err = nvs_get_u8(handler, "has_data", &has_data);
    nvs_close(handler);

    if (err != ESP_OK) {
        ESP_LOGE("EEPROM", "Error (%s) reading!\n", esp_err_to_name(err));
    }

    ESP_LOGI("EEPROM", "WiFi configured?: %d", has_data);
    return has_data == 0 ? true : false;
}

void set_wifi_configured(uint8_t has_data) {
    nvs_handle_t handler = eeprom_handler();

    esp_err_t err = nvs_set_u8(handler, "has_data", has_data);
    nvs_close(handler);

    if (err != ESP_OK) {
        ESP_LOGE("EEPROM", "Error (%s) writing!\n", esp_err_to_name(err));
    }
    ESP_LOGI("EEPROM", "WiFi configured!");
}

void save_wifi_credentials(const char *ssid, const char *password) {
    nvs_handle_t handler = eeprom_handler();

    esp_err_t err = nvs_set_str(handler, "ssid", ssid);
    if (err != ESP_OK) {
        ESP_LOGE("EEPROM", "Error (%s) writing!\n", esp_err_to_name(err));
    }

    err = nvs_set_str(handler, "password", password);
    if (err != ESP_OK) {
        ESP_LOGE("EEPROM", "Error (%s) writing!\n", esp_err_to_name(err));
    }

    ESP_LOGI("EEPROM", "Credentials saved: SSID: %s | Password: %s", ssid, password);
    nvs_close(handler);
}

wifi_credentials_t load_wifi_credentials() {
    nvs_handle_t handler = eeprom_handler();
    wifi_credentials_t credentials;

    size_t ssid_size = sizeof(credentials.ssid);
    size_t password_size = sizeof(credentials.password);

    esp_err_t err = nvs_get_str(handler, "ssid", credentials.ssid, &ssid_size);
    if (err != ESP_OK) {
        ESP_LOGE("EEPROM", "Error (%s) reading!\n", esp_err_to_name(err));
    }

    err = nvs_get_str(handler, "password", credentials.password, &password_size);
    if (err != ESP_OK) {
        ESP_LOGE("EEPROM", "Error (%s) reading!\n", esp_err_to_name(err));
    }

    nvs_close(handler);
    ESP_LOGI("EEPROM", "Credentials loaded: SSID: %s | Password: %s", credentials.ssid, credentials.password);
    return credentials;
}

void reset_wifi_credentials() {
    nvs_handle_t handler = eeprom_handler();
    esp_err_t err = nvs_erase_key(handler, "ssid");
    if (err != ESP_OK) {
        ESP_LOGE("EEPROM", "Error (%s) erasing!\n", esp_err_to_name(err));
    }

    err = nvs_erase_key(handler, "password");
    if (err != ESP_OK) {
        ESP_LOGE("EEPROM", "Error (%s) erasing!\n", esp_err_to_name(err));
    }

    // reset the flag, that indicates the wifi isn't configured
    set_wifi_configured(1);
    
    nvs_close(handler);
    ESP_LOGI("EEPROM", "Credentials erased!");
}