#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <esp_log.h>
#include <esp_wifi.h>
#include <nvs_flash.h>
#include <wifi_esp.h>
#include <string.h>
#include <esp_mac.h>
#include <lwip/sockets.h>

#define DEFAULT_SCAN_LIST_SIZE 10
#define AP_IP "4.3.2.1"
#define AP_GATEWAY "4.3.2.1"
#define AP_NETMASK "255.255.255.0"

#define DNS_SERVER_IP "4.3.2.1"
#define DNS_PORT 53
#define BUFFER_SIZE 512

EventGroupHandle_t wifi_event_group;
const int WIFI_CONNECTED_BIT = BIT0;

void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        esp_wifi_connect();
        ESP_LOGI("WIFI", "Retrying to connect to the WiFi network...");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

esp_err_t wifi_init(const char* ssid, const char* pass) {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    wifi_event_group = xEventGroupCreate();
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, &instance_got_ip));

    wifi_config_t wifi_config = {};
    strcpy((char*)wifi_config.sta.ssid, ssid);
    strcpy((char*)wifi_config.sta.password, pass);

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    wifi_ap_record_t ap_info;
    ret = esp_wifi_sta_get_ap_info(&ap_info);
    if (ret == ESP_OK) {
        ESP_LOGI("WIFI", "Connected to AP SSID: %s", ap_info.ssid);
    } else {
        ESP_LOGI("WIFI", "Not connected to any AP");
    }

    ESP_LOGI("WIFI", "wifi_init_sta finished.");
    EventBits_t bits = xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_BIT, pdFALSE, pdTRUE, portMAX_DELAY);

    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI("WIFI", "connected to ap SSID:%s password:%s", ssid, pass);
    } else {
        ESP_LOGI("WIFI", "Failed to connect to SSID:%s, password:%s", ssid, pass);
        return ESP_FAIL;
    }

    return ESP_OK;
}

void wifi_scan() {
    ESP_LOGI("WIFI_SCAN", "Scanning for WiFi networks...");
    wifi_scan_config_t scan_config = {
        .ssid = NULL,
        .bssid = NULL,
        .channel = 0,
        .show_hidden = true,
        .scan_type = WIFI_SCAN_TYPE_ACTIVE,
        .scan_time = {
            .active = {
                .min = 100,
                .max = 200
            },
        }
    };
    ESP_ERROR_CHECK(esp_wifi_scan_start(&scan_config, true));

    uint16_t ap_count = 0;
    esp_wifi_scan_get_ap_num(&ap_count);
    wifi_ap_record_t ap_info[ap_count];
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&ap_count, ap_info));

    for (int i = 0; i < ap_count; i++) {
        ESP_LOGI("WIFI", "SSID \t\t%s", ap_info[i].ssid);
        ESP_LOGI("WIFI", "RSSI \t\t%d", ap_info[i].rssi);
        ESP_LOGI("WIFI", "Channel \t\t%d", ap_info[i].primary);
    }
}

void dns_server_task(void *pvParameters) {
    int sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    uint8_t buffer[BUFFER_SIZE];

    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        ESP_LOGE("DNS", "Erro ao criar socket");
        vTaskDelete(NULL);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(DNS_PORT);

    if (bind(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        ESP_LOGE("DNS", "Erro ao vincular socket");
        close(sock);
        vTaskDelete(NULL);
    }

    ESP_LOGI("DNS", "DNS server started on port: %d", DNS_PORT);

    while (1) {
        int len = recvfrom(sock, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&client_addr, &client_addr_len);
        if (len < 0) {
            ESP_LOGE("DNS", "Error receiving data");
            break;
        }

        buffer[2] |= 0x80; 
        buffer[7] = 1;     

        uint8_t response[] = {
            0xc0, 0x0c,       // Nome
            0x00, 0x01,       // Tipo: A (Host Address)
            0x00, 0x01,       // Classe: IN (Internet)
            0x00, 0x00, 0x00, 0x3c, // TTL (60 segundos)
            0x00, 0x04,       // Comprimento dos dados
            4, 3, 2, 1        // Endereço IP: 4.3.2.1
        };
        memcpy(buffer + len, response, sizeof(response));
        len += sizeof(response);

        sendto(sock, buffer, len, 0, (struct sockaddr *)&client_addr, client_addr_len);
    }

    close(sock);
    vTaskDelete(NULL);
}

void start_wifi_ap() {
    ESP_LOGI("WIFI", "Starting WiFi AP...");

    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_ap();

    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    wifi_config_t wifi_config = {
        .ap = {
            .ssid_len = 0,
            .channel = 6,
            .authmode = WIFI_AUTH_OPEN,
            .max_connection = 4
        }
    };
    snprintf((char *)wifi_config.ap.ssid, sizeof(wifi_config.ap.ssid), "DC%02X%02X%02X%02X%02X%02X",
            mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    wifi_config.ap.ssid_len = strlen((char *)wifi_config.ap.ssid);

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    esp_netif_ip_info_t ip_info;
    esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_AP_DEF");
    esp_netif_ip_info_t info = {
        .ip = esp_ip4addr_aton(AP_IP),
        .netmask = esp_ip4addr_aton(AP_NETMASK),
        .gw = esp_ip4addr_aton(AP_GATEWAY),
    };
    
    esp_netif_dhcps_stop(netif); // Stop DHCP
    esp_netif_set_ip_info(netif, &info);
    esp_netif_dhcps_start(netif);

    ESP_LOGI("WIFI", "SSID: %s", wifi_config.ap.ssid);
    xTaskCreate(dns_server_task, "dns_server_task", 4096, NULL, 5, NULL);
}