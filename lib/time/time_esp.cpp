#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <esp_log.h>
#include <esp_sntp.h>

#define TIME_BUFFER_SIZE 64

void time_sync_notification_cb(struct timeval *tv) {
    ESP_LOGI("TIME", "Time synchronization event received");
}

esp_err_t sync_time() {
    ESP_LOGI("TIME", "Initializing SNTP");

    // On linux, this will also set the timezone
    // esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    // esp_sntp_setservername(0, (char *) "pool.ntp.org");
    // sntp_set_time_sync_notification_cb(time_sync_notification_cb);
    // esp_sntp_init();

    // On windows, this will not set the timezone
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "pool.ntp.org");
    sntp_set_time_sync_notification_cb(time_sync_notification_cb);
    sntp_init();

    setenv("TZ", "BRT3", 1);
    tzset();

    time_t now = 0;
    int retry = 0;
    struct tm timeinfo = { 0 };
    const int retry_count = 10;

    while (timeinfo.tm_year < (2016 - 1900) && ++retry < retry_count) {
        ESP_LOGI("TIME", "Waiting for system time to be set... (%d/%d)", retry, retry_count);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
        time(&now);
        localtime_r(&now, &timeinfo);
    }

    if (retry == retry_count) {
        ESP_LOGE("TIME", "Failed to synchronize time");
        return ESP_FAIL;
    } else {
        ESP_LOGI("TIME", "Time synchronized successfully");
        return ESP_OK;
    }
}

char* get_format_time() {

    time_t rawtime;
    time(&rawtime);

    static char message[TIME_BUFFER_SIZE];
    struct tm timeinfo;

    localtime_r(&rawtime, &timeinfo);
    strftime(message, TIME_BUFFER_SIZE, "%d/%m/%Y %H:%M:%S", &timeinfo);

    return message;
}