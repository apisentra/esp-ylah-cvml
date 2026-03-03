/*
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdio.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_camera.h"
#include "esp_log.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_camera.h"
#include "camera_pins.h"
#include "camera.h"
#include "esp_sleep.h"
#include "cv.h"
#include "tasks/take_images.h"
#include <string.h>
#include "esp_err.h"
#include "esp_check.h"
#include "tasks/write_sd.h"
#include "helpers.h"

#include "esp_log.h"
#include "esp_bt.h"
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_bt_main.h"
#include "nvs_flash.h"
#include "nvs_sec_provider.h"
#include "ble_config_server.h"

RTC_DATA_ATTR int boot_count = 0;

bool save_to_sd_enabled = SAVE_TO_SD;
bool sd_fail_mount = false;

static char path[32]; // fewer stack bytes

esp_err_t save_to_sd(char *path, uint8_t *data, size_t len)
{
    if (!sd_fail_mount && save_to_sd_enabled)
    {
        ESP_LOGI("SD", "Saving %i bytes to SD at :%s", len, path);
        return write_to_sd(path, data, len);
    }
    else if (!save_to_sd_enabled)
    {
        return ESP_OK;
    }
    else
    {
        return ESP_ERR_INVALID_STATE;
    }
}

void app_main(void)
{
    // Start BLE GATT server
    ble_config_server_init();

    ESP_LOGI("BLE", "BLE Initialized - Ready to create services/characteristics");

#ifdef SAVE_TO_SD
    if (mount_sdcard() != ESP_OK)
    {
        sd_fail_mount = true;
        ESP_LOGE("SD_CARD", "Failed to Mount SD Card");
    }
#endif

    ESP_LOGI("PSRAM", "Free: %u bytes",
             heap_caps_get_free_size(MALLOC_CAP_SPIRAM));

    boot_count++;

    ESP_ERROR_CHECK(init_camera_x(PIXFORMAT_YUV422, FRAMESIZE_SVGA));

    for (int i = 0; i < 3; i++)
    {
        esp_camera_fb_return(esp_camera_fb_get());
    }

    camera_fb_t *rgb565 = esp_camera_fb_get();
    IF_CAM_FB_NULL(rgb565);
    LOG_CAM_FRAME_DETAILS(rgb565);

    uint8_t *out = NULL;
    size_t len = 0;

    frame2jpg(rgb565, 80, &out, &len);

    ESP_LOGI("APP", "JPEG Conversion completed : size:%i", len);

    esp_camera_fb_return(rgb565);

    sprintf(path, "/sdcard/%i.jpg", boot_count);
    ESP_ERROR_CHECK(save_to_sd(path, out, len));

    free(out);

    ESP_LOGI("APP", "Freed malloc for JPEG conversion.", len);

    multi_heap_info_t info;
    heap_caps_get_info(&info, MALLOC_CAP_SPIRAM);
    ESP_LOGI("MEM", "PSRAM total: %u, free: %u, largest block: %u", info.total_free_bytes, info.total_allocated_bytes, info.largest_free_block);

    multi_heap_info_t infoDram;
    heap_caps_get_info(&infoDram, MALLOC_CAP_8BIT);
    ESP_LOGI("MEM", "DRAM total: %u, free: %u, largest block: %u", infoDram.total_free_bytes, infoDram.total_allocated_bytes, infoDram.largest_free_block);

    ESP_LOGI("MAIN", "DONE - Going to Sleep!");

    // esp_deep_sleep(30000000);
}
