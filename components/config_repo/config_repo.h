#pragma once
#include "esp_err.h"

esp_err_t nvs_config_init(void);
esp_err_t nvs_snvs_init(void);

// Getters
esp_err_t nvs_get_server_url(char *buffer, size_t max_len);
esp_err_t nvs_get_threshold(int32_t *value);
esp_err_t nvs_get_ssid(char *buffer, size_t max_len);
esp_err_t nvs_get_wifi_password(char *buffer, size_t max_len);

// Setters
esp_err_t nvs_set_server_url(const char *value);
esp_err_t nvs_set_threshold(int32_t value);
esp_err_t nvs_set_ssid(const char *value);
esp_err_t nvs_set_wifi_password(const char *value);