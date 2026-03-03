/**
 * config_repo.c
 */

#include "config_repo.h"
#include "nvs_util.h"
#include "esp_log.h"
#include <stdio.h>
#include <stdlib.h>

#define SECURE_PARTITION "snvs"
#define SECURE_NAMESPACE "wifi_cfg"
#define DEFAULT_NAMESPACE "ble_cfg"

static const char *TAG = "NVS_CONFIG";
static const char *NAMESPACE = "ble_cfg";

esp_err_t nvs_config_init(void)
{
    return nvs_util_init_default();
}

esp_err_t nvs_snvs_init(){
    return nvs_util_init_secure(SECURE_PARTITION);
}

// Strings
esp_err_t nvs_get_server_url(char *buffer, size_t max_len)
{
    return nvs_util_get_str_from_partition(NULL, NAMESPACE, "server_url", buffer, max_len);
}

esp_err_t nvs_set_server_url(const char *value)
{
    if (!value || strlen(value) == 0) return ESP_ERR_INVALID_ARG;
    return nvs_util_set_str_from_partition(NULL, NAMESPACE, "server_url", value);
}

esp_err_t nvs_get_ssid(char *buffer, size_t max_len)
{
    return nvs_util_get_str_from_partition(SECURE_PARTITION, SECURE_NAMESPACE, "ssid", buffer, max_len);
}

esp_err_t nvs_set_ssid(const char *value)
{
    if (!value || strlen(value) == 0) return ESP_ERR_INVALID_ARG;
    return nvs_util_set_str_from_partition(SECURE_PARTITION, SECURE_NAMESPACE, "ssid", value);
}

esp_err_t nvs_get_wifi_password(char *buffer, size_t max_len)
{
    return nvs_util_get_str_from_partition(SECURE_PARTITION, SECURE_NAMESPACE, "wifi_password", buffer, max_len);
}

esp_err_t nvs_set_wifi_password(const char *value)
{
    if (!value || strlen(value) == 0) return ESP_ERR_INVALID_ARG;
    return nvs_util_set_str_from_partition(SECURE_PARTITION, SECURE_NAMESPACE, "wifi_password", value);
}

// Integer
esp_err_t nvs_get_threshold(int32_t *value)
{
    char buf[16];
    esp_err_t ret = nvs_util_get_str_from_partition(NULL, NAMESPACE, "threshold", buf, sizeof(buf));
    if (ret != ESP_OK) return ret;

    *value = atoi(buf);
    return ESP_OK;
}

esp_err_t nvs_set_threshold(int32_t value)
{
    char buf[16];
    snprintf(buf, sizeof(buf), "%ld", value);
    return nvs_util_set_str_from_partition(NULL, NAMESPACE, "threshold", buf);
}