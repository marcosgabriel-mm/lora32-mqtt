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
    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, (char *) "200.160.7.186");
    sntp_set_time_sync_notification_cb(time_sync_notification_cb);
    esp_sntp_init();

    // On windows, this will not set the timezone
    // sntp_setoperatingmode(SNTP_OPMODE_POLL);
    // sntp_setservername(0, "200.160.7.186");
    // sntp_set_time_sync_notification_cb(time_sync_notification_cb);
    // sntp_init();

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

time_t get_epoch_time() {
    time_t now;
    time(&now);
    return now;
}

int64_t get_epoch_time_ms() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return ((int64_t)tv.tv_sec * 1000) + (tv.tv_usec / 1000);
}

int64_t get_epoch_time_us() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return ((int64_t)tv.tv_sec * 1000000) + tv.tv_usec;
}

char* formart_timestamp_us(int64_t timestamp_us) {
    static char time_buffer[TIME_BUFFER_SIZE];
    
    time_t seconds = timestamp_us / 1000000;
    int microseconds = timestamp_us % 1000000;
    int milliseconds = microseconds / 1000;
    
    struct tm timeinfo;
    localtime_r(&seconds, &timeinfo);
    
    // Formato: YYYY-MM-DD HH:MM:SS:mmm
    snprintf(time_buffer, sizeof(time_buffer), "%04d-%02d-%02d %02d:%02d:%02d:%03d",
             timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
             timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec, milliseconds);
    
    return time_buffer;
}

char* get_format_time() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    
    struct tm timeinfo;
    localtime_r(&tv.tv_sec, &timeinfo);
    
    static char time_buffer[TIME_BUFFER_SIZE];
    int milliseconds = tv.tv_usec / 1000;
    
    // Formato: YYYY-MM-DD HH:MM:SS:mmm
    snprintf(time_buffer, sizeof(time_buffer), "%04d-%02d-%02d %02d:%02d:%02d:%03d",
             timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
             timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec, milliseconds);
    
    return time_buffer;
}