#include <wifi_credentials_page/html_documents.h>
#include <wifi_credentials_page/wifi_credentials.h>
#include <esp_http_server.h>
#include <esp_log.h>
#include <sys/param.h>
#include <wifi_esp.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

void url_decode(char *decoded, const char *encoded, size_t max_len) {
    char *dest = decoded;
    const char *src = encoded;
    while (*src && (dest - decoded) < (max_len - 1)) {
        if (*src == '%' && isxdigit((unsigned char)src[1]) && isxdigit((unsigned char)src[2])) {
            char hex[3] = { src[1], src[2], '\0' };
            *dest++ = (char)strtol(hex, NULL, 16);
            src += 3;
        } else if (*src == '+') {
            *dest++ = ' ';  // Substituir '+' por espaço
            src++;
        } else {
            *dest++ = *src++;
        }
    }
    *dest = '\0';
}

esp_err_t root_handler(httpd_req_t *req) {
    httpd_resp_set_type(req, "text/html");
    return httpd_resp_send(req, WIFI_CONFIG_PAGE, strlen(WIFI_CONFIG_PAGE));
}

static wifi_credentials_t last_credentials = {0};
esp_err_t set_wifi_handler(httpd_req_t *req) {
    char query[100] = {0}, decoded_ssid[32] = {0}, decoded_password[64] = {0};

    if (httpd_req_get_url_query_str(req, query, sizeof(query)) == ESP_OK) {
        char param[32];
        if (httpd_query_key_value(query, "ssid", param, sizeof(param)) == ESP_OK) {
            url_decode(decoded_ssid, param, sizeof(decoded_ssid));
            strcpy(last_credentials.ssid, decoded_ssid);
        }
        if (httpd_query_key_value(query, "password", param, sizeof(param)) == ESP_OK) {
            url_decode(decoded_password, param, sizeof(decoded_password));
            strcpy(last_credentials.password, decoded_password);
        }
        last_credentials.connected = true;
    }
    ESP_LOGI("WIFI", "SSID: %s | Password: %s", last_credentials.ssid, last_credentials.password);
    
    httpd_resp_set_type(req, "text/html");
    return httpd_resp_send(req, WIFI_CONFIGURED_SUCCESSFUL, strlen(WIFI_CONFIGURED_SUCCESSFUL));
}

esp_err_t redirect_handler(httpd_req_t *req) {
    httpd_resp_set_status(req, "302 Found");
    httpd_resp_set_hdr(req, "Location", "http://4.3.2.1");
    return httpd_resp_send(req, NULL, 0);
}

esp_err_t connect_test_handler(httpd_req_t *req) {
    httpd_resp_set_type(req, "text/html");
    return httpd_resp_send(req, "http://logout.net", 18);
}

esp_err_t not_found_handler(httpd_req_t *req, httpd_err_code_t err) {
    httpd_resp_set_status(req, "302 Found");
    httpd_resp_set_hdr(req, "Location", "http://4.3.2.1");
    return httpd_resp_send(req, NULL, 0);
}

wifi_credentials_t wifi_credentials(void) {
    return last_credentials;
} 

void start_webserver(void) {
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    if (httpd_start(&server, &config) == ESP_OK) {

        httpd_uri_t connect_test = {
            .uri      = "/connecttest.txt",
            .method   = HTTP_GET,
            .handler  = connect_test_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(server, &connect_test);

        httpd_uri_t redirect = {
            .uri      = "/redirect",
            .method   = HTTP_GET,
            .handler  = redirect_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(server, &redirect);

        httpd_uri_t hotspot_detect_html = {
            .uri      = "/hotspot-detect.html",
            .method   = HTTP_GET,
            .handler  = redirect_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(server, &hotspot_detect_html);

        httpd_uri_t canonical_html = {
            .uri      = "/canonical.html",
            .method   = HTTP_GET,
            .handler  = redirect_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(server, &canonical_html);

        httpd_uri_t generate_204 = {
            .uri      = "/generate_204",
            .method   = HTTP_GET,
            .handler  = redirect_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(server, &generate_204);

        httpd_uri_t ncsi_txt = {
            .uri      = "/ncsi.txt",
            .method   = HTTP_GET,
            .handler  = redirect_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(server, &ncsi_txt);

        httpd_uri_t root = {
            .uri      = "/",
            .method   = HTTP_GET,
            .handler  = root_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(server, &root);

        httpd_uri_t set_wifi = {
            .uri      = "/set_wifi",
            .method   = HTTP_GET,
            .handler  = set_wifi_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(server, &set_wifi);
        httpd_register_err_handler(server, HTTPD_404_NOT_FOUND, not_found_handler);
    }
}
