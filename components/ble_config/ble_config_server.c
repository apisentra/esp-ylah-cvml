#include "ble_config_server.h"
#include "esp_log.h"
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_gatt_common_api.h"
#include <string.h>
#include "config_repo.h"
#include "esp_sntp.h"
#include <time.h>
#include <sys/time.h>

#define TAG "BLE_CONFIG"

#define BLE_MAX_MTU 247

// UUIDs
#define CONFIG_SERVICE_UUID 0x00FF
#define SERVER_URL_CHAR_UUID 0xFF01
#define THRESHOLD_CHAR_UUID 0xFF02
#define SSID_CHAR_UUID 0xFF03
#define WIFI_PASSWORD_CHAR_UUID 0xFF04
#define SET_RTC_EPOCH_TIME 0xFF05

#define MAX_SERVER_LEN 128

#define MAX_SSID_LEN 128
static char ssid[MAX_SSID_LEN] = {0};

#define MAX_WIFI_PASSOWRD_LEN 128
static char b_wifi_password[MAX_WIFI_PASSOWRD_LEN] = {0};

/**
 * On creation of the characteristic, store the handle number here in order to be able to match them to the characteristic.
 */
static uint16_t server_url_handle = 0;
static uint16_t threshold_handle = 0;
static uint16_t ssid_handle = 0;
static uint16_t wifi_password_handle = 0;
static uint16_t rtc_epoch_handle = 0;

static uint16_t service_handle;
static esp_gatt_if_t gatts_if_global;

static char server_url[128] = "https://api.example.com";
static int threshold = 50;
static uint32_t rtc_epoch_time = 0;

static esp_bt_uuid_t service_uuid = {
    .len = ESP_UUID_LEN_16,
    .uuid = {.uuid16 = CONFIG_SERVICE_UUID}};

static esp_bt_uuid_t server_url_uuid = {
    .len = ESP_UUID_LEN_16,
    .uuid = {.uuid16 = SERVER_URL_CHAR_UUID}};

static esp_bt_uuid_t threshold_uuid = {
    .len = ESP_UUID_LEN_16,
    .uuid = {.uuid16 = THRESHOLD_CHAR_UUID}};

static esp_bt_uuid_t ssid_uid = {
    .len = ESP_UUID_LEN_16,
    .uuid = {.uuid16 = SSID_CHAR_UUID}};

static esp_bt_uuid_t wifi_password = {
    .len = ESP_UUID_LEN_16,
    .uuid = {.uuid16 = WIFI_PASSWORD_CHAR_UUID}};

static esp_bt_uuid_t rtc_epoch_time_uuid = {
    .len = ESP_UUID_LEN_16,
    .uuid = {.uuid16 = SET_RTC_EPOCH_TIME}};

static void start_advertising(void)
{
    esp_ble_adv_data_t adv_data = {
        .set_scan_rsp = false,
        .include_name = true,
        .include_txpower = true,
        .service_uuid_len = 2,
        .p_service_uuid = (uint8_t[]){0xFF, 0x00}, // little endian
        .flag = ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT,
    };

    esp_ble_gap_config_adv_data(&adv_data);

    esp_ble_adv_params_t adv_params = {
        .adv_int_min = 0x20,
        .adv_int_max = 0x40,
        .adv_type = ADV_TYPE_IND,
        .own_addr_type = BLE_ADDR_TYPE_PUBLIC,
        .channel_map = ADV_CHNL_ALL,
        .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
    };

    esp_ble_gap_start_advertising(&adv_params);
}

static void gatts_event_handler(esp_gatts_cb_event_t event,
                                esp_gatt_if_t gatts_if,
                                esp_ble_gatts_cb_param_t *param)
{

    switch (event)
    {

    case ESP_GATTS_DISCONNECT_EVT:
        ESP_LOGI(TAG, "Client disconnected, restarting advertising");
        start_advertising();
        break;

    case ESP_GATTS_READ_EVT:
    {
        uint16_t handle = param->read.handle;
        uint16_t len = 0;
        uint8_t *value_ptr = NULL;

        ESP_LOGI(TAG, "ESP_GATTS_READ_EVT : %i", param->read.handle);

        // Match the characteristic handle
        if (handle == server_url_handle)
        {
            value_ptr = (uint8_t *)server_url;
            len = strlen(server_url);

            esp_gatt_rsp_t rsp;
            memset(&rsp, 0, sizeof(rsp)); // zero init

            rsp.attr_value.handle = handle;
            rsp.attr_value.len = len;

            // Copy the actual value into the array
            if (value_ptr && len > 0)
            {
                memcpy(rsp.attr_value.value, value_ptr, len);
            }

            esp_ble_gatts_send_response(
                gatts_if,
                param->read.conn_id,
                param->read.trans_id,
                ESP_GATT_OK,
                &rsp);
        }
        else if (handle == threshold_handle)
        {
            value_ptr = (uint8_t *)&threshold;
            len = sizeof(threshold);

            esp_gatt_rsp_t rsp;
            memset(&rsp, 0, sizeof(rsp)); // zero init

            rsp.attr_value.handle = handle;
            rsp.attr_value.len = len;

            // Copy the actual value into the array
            if (value_ptr && len > 0)
            {
                memcpy(rsp.attr_value.value, value_ptr, len);
            }

            esp_ble_gatts_send_response(
                gatts_if,
                param->read.conn_id,
                param->read.trans_id,
                ESP_GATT_OK,
                &rsp);
        }
        else if (handle == ssid_handle)
        {
            ESP_LOGI(TAG, "Value SSID : %s", ssid);
            value_ptr = (uint8_t *)ssid;
            len = strlen(ssid);

            esp_gatt_rsp_t rsp;
            memset(&rsp, 0, sizeof(rsp)); // zero init

            rsp.attr_value.handle = handle;
            rsp.attr_value.len = len;

            // Copy the actual value into the array
            if (value_ptr && len > 0)
            {
                memcpy(rsp.attr_value.value, value_ptr, len);
            }

            esp_ble_gatts_send_response(
                gatts_if,
                param->read.conn_id,
                param->read.trans_id,
                ESP_GATT_OK,
                &rsp);
        }
        else if (handle == wifi_password_handle)
        {
            ESP_LOGW(TAG, "Client attempting to read the wifi password characteristic : Denied.");
            // Sensitive: block read or return nothing
            value_ptr = NULL;
            len = 0;

            esp_gatt_rsp_t rsp;
            memset(&rsp, 0, sizeof(rsp)); // zero init

            rsp.attr_value.handle = handle;
            rsp.attr_value.len = len;

            // Copy the actual value into the array
            if (value_ptr && len > 0)
            {
                memcpy(rsp.attr_value.value, value_ptr, len);
            }

            esp_ble_gatts_send_response(
                gatts_if,
                param->read.conn_id,
                param->read.trans_id,
                ESP_GATT_OK,
                &rsp);
        }
        else if (handle == rtc_epoch_handle)
        {
            ESP_LOGI(TAG, "Client Reading the RTC Epoch Time.");

            // Get current RTC/Unix time
            time_t now;
            time(&now);

            // Prepare GATT response
            esp_gatt_rsp_t rsp;
            memset(&rsp, 0, sizeof(rsp));

            rsp.attr_value.handle = handle;
            rsp.attr_value.len = sizeof(now); // 8 bytes on ESP-IDF 64-bit time_t

            memcpy(rsp.attr_value.value, &now, sizeof(now));

            ESP_LOGI(TAG, "Epoch Time Returning to BLE Client: %ld", (long)now);

            // Send the response
            esp_ble_gatts_send_response(
                gatts_if,
                param->read.conn_id,
                param->read.trans_id,
                ESP_GATT_OK,
                &rsp);
        }

        break;
    }

    case ESP_GATTS_WRITE_EVT:
    {
        ESP_LOGI(TAG, "Write event, len=%d handle=%i", param->write.len, param->write.handle);

        if (!param->write.is_prep)
        {
            // Always respond to write requests
            esp_ble_gatts_send_response(
                gatts_if,
                param->write.conn_id,
                param->write.trans_id,
                ESP_GATT_OK,
                NULL);
        }

        // Example: store wifi password if written
        if (param->write.handle == wifi_password_handle)
        {
            int len = param->write.len;
            if (len < (MAX_WIFI_PASSOWRD_LEN - 1))
            {
                memcpy(b_wifi_password, param->write.value, len);
                b_wifi_password[param->write.len] = '\0';
                nvs_set_wifi_password(b_wifi_password);
                memcpy(b_wifi_password, "", 1);
                ESP_LOGI(TAG, "Updated wifi_password.", server_url);

                esp_gatt_rsp_t rsp;
                memset(&rsp, 0, sizeof(rsp));                // zero-initialize
                rsp.attr_value.handle = param->write.handle; // must set handle
                rsp.attr_value.len = 0;                      // no data needed for simple write

                // Update characteristic length in the BLE stack
                esp_attr_value_t char_val = {
                    .attr_max_len = sizeof(b_wifi_password),
                    .attr_len = len,
                    .attr_value = (uint8_t *)b_wifi_password};
                esp_ble_gatts_set_attr_value(wifi_password_handle, len, (uint8_t *)b_wifi_password);

                // Send OK response to client
                esp_ble_gatts_send_response(
                    gatts_if,
                    param->write.conn_id,
                    param->write.trans_id,
                    ESP_GATT_OK, // indicate success
                    &rsp         // no additional data needed
                );
            }
            else
            {
                // Send error response to client : Invalid attribute length.
                esp_ble_gatts_send_response(
                    gatts_if,
                    param->write.conn_id,
                    param->write.trans_id,
                    ESP_GATT_INVALID_ATTR_LEN,
                    NULL);
            }
        }

        // Example: store server_url if written
        if (param->write.handle == server_url_handle)
        {
            int len = param->write.len;
            if (len < sizeof(server_url))
            {
                memcpy(server_url, param->write.value, len);
                server_url[len] = '\0';
                ESP_LOGI(TAG, "Updated server_url: %s", server_url);

                // Update characteristic length in the BLE stack
                esp_attr_value_t char_val = {
                    .attr_max_len = sizeof(server_url),
                    .attr_len = len,
                    .attr_value = (uint8_t *)server_url};
                esp_ble_gatts_set_attr_value(server_url_handle, len, (uint8_t *)server_url);
            }
        }
        else if (param->write.handle == ssid_handle)
        {
            int len = param->write.len;

            if (param->write.len >= sizeof(ssid) - 1)
            {
                ESP_LOGW(TAG, "SSID too long (%d bytes)", param->write.len);

                // Send error response to client : Invalid attribute length.
                esp_ble_gatts_send_response(
                    gatts_if,
                    param->write.conn_id,
                    param->write.trans_id,
                    ESP_GATT_INVALID_ATTR_LEN,
                    NULL);
                break;
            }

            if (len < sizeof(ssid) - 1)
            {
                memcpy(ssid, param->write.value, len);
                ssid[len] = '\0';
                ESP_LOGI(TAG, "Updated SSID: %s", ssid);
                nvs_set_ssid(ssid);

                esp_gatt_rsp_t rsp;
                memset(&rsp, 0, sizeof(rsp));                // zero-initialize
                rsp.attr_value.handle = param->write.handle; // must set handle
                rsp.attr_value.len = 0;                      // no data needed for simple write

                // Update characteristic length in the BLE stack
                esp_attr_value_t char_val = {
                    .attr_max_len = sizeof(ssid),
                    .attr_len = len,
                    .attr_value = (uint8_t *)ssid};
                esp_ble_gatts_set_attr_value(ssid_handle, len, (uint8_t *)ssid);

                // Send OK response to client
                esp_ble_gatts_send_response(
                    gatts_if,
                    param->write.conn_id,
                    param->write.trans_id,
                    ESP_GATT_OK, // indicate success
                    &rsp         // no additional data needed
                );
            }
        }
        else if (param->write.handle == rtc_epoch_handle)
        {
            uint32_t value;
            memcpy(&value, param->write.value, sizeof(uint32_t));
            rtc_epoch_time = value;

            struct timeval tv = {
                .tv_sec = rtc_epoch_time,
                .tv_usec = 0};

            settimeofday(&tv, NULL);

            time_t now;
            time(&now);

            struct tm utc_tm;
            gmtime_r(&now, &utc_tm);

            ESP_LOGI(TAG, "Updated RTC UTC time: %s", asctime(&utc_tm));
            // nvs_set_ssid(ssid);

            esp_gatt_rsp_t rsp;
            memset(&rsp, 0, sizeof(rsp));                // zero-initialize
            rsp.attr_value.handle = param->write.handle; // must set handle
            rsp.attr_value.len = 0;                      // no data needed for simple write

            // Send OK response to client
            esp_ble_gatts_send_response(
                gatts_if,
                param->write.conn_id,
                param->write.trans_id,
                ESP_GATT_OK, // indicate success
                &rsp         // no additional data needed
            );
        }

        break;
    }

    case ESP_GATTS_SET_ATTR_VAL_EVT:
    {
        ESP_LOGI(TAG, "ESP_GATTS_SET_ATTR_VAL_EVT");
        break;
    }
    case ESP_GATTS_CONNECT_EVT:
    {
        ESP_LOGI(TAG, "GATTS CONNECTED EVENT!");

        esp_err_t ret = esp_ble_gatt_set_local_mtu(BLE_MAX_MTU);
        if (ret != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to set local MTU: 0x%x", ret);
        }
        else
        {
            ESP_LOGI(TAG, "Local MTU set to %d", BLE_MAX_MTU);
        }

        break;
    }

    case ESP_GATTS_ADD_CHAR_EVT:
        ESP_LOGI(TAG, "ADD_CHAR_EVT, handle = %d",
                 param->add_char.attr_handle);

        if (param->add_char.status != ESP_GATT_OK)
        {
            ESP_LOGE(TAG, "Add char failed");
            break;
        }
        ESP_LOGI(TAG, "Characteristic Handle UUID Added Event UUID : %i", param->add_char.char_uuid.uuid.uuid16);
        if (param->add_char.char_uuid.len == ESP_UUID_LEN_16)
        {

            uint16_t uuid = param->add_char.char_uuid.uuid.uuid16;

            if (uuid == server_url_uuid.uuid.uuid16)
            {

                server_url_handle = param->add_char.attr_handle;
                ESP_LOGI(TAG, "Server URL Characteristic Added Characteristic UUID = %i Handle = %i", uuid, server_url_handle);
            }
            else if (uuid == threshold_uuid.uuid.uuid16)
            {
                threshold_handle = param->add_char.attr_handle;
                ESP_LOGI(TAG, "Threshold Characteristic Added Characteristic UUID = %i Handle = %i", &uuid, threshold_handle);
            }
            else if (uuid == ssid_uid.uuid.uuid16)
            {
                ssid_handle = param->add_char.attr_handle;
                ESP_LOGI(TAG, "SSID Characteristic Added Characteristic UUID = %i Handle = %i", &uuid, ssid_handle);
            }
            else if (uuid == wifi_password.uuid.uuid16)
            {
                wifi_password_handle = param->add_char.attr_handle;
                ESP_LOGI(TAG, "WifiPassword Characteristic Added : Characteristic UUID = %i Handle = %i", &uuid, wifi_password_handle);
            }
            else if (uuid == rtc_epoch_time_uuid.uuid.uuid16)
            {
                rtc_epoch_handle = param->add_char.attr_handle;
                ESP_LOGI(TAG, "RTCTimeEpoch Characteristic Added : Characteristic UUID = %i Handle = %i", &uuid, rtc_epoch_handle);
            }
        }

        break;

    case ESP_GATTS_MTU_EVT:
        ESP_LOGI(TAG, "Negotiated MTU = %d", param->mtu.mtu);
        break;

    case ESP_GATTS_REG_EVT:
    {
        gatts_if_global = gatts_if;
        ESP_LOGI(TAG, "Setting BLE Device Name.");
        ESP_ERROR_CHECK(esp_ble_gap_set_device_name("APISENTRAYLAH"));

        esp_gatt_srvc_id_t service_id = {
            .is_primary = true,
            .id.inst_id = 0,
            .id.uuid = service_uuid};

        esp_ble_gatts_create_service(gatts_if, &service_id, 12);
        break;
    }

    case ESP_GATTS_CREATE_EVT:
    {
        service_handle = param->create.service_handle;

        esp_ble_gatts_start_service(service_handle);

        esp_attr_value_t char_val = {
            .attr_max_len = sizeof(server_url),
            .attr_len = strlen(server_url),
            .attr_value = (uint8_t *)server_url};

        esp_ble_gatts_add_char(service_handle,
                               &server_url_uuid,
                               ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
                               ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_WRITE,
                               &char_val,
                               NULL);

        char_val.attr_max_len = sizeof(threshold);
        char_val.attr_len = sizeof(threshold);
        char_val.attr_value = (uint8_t *)&threshold;

        esp_ble_gatts_add_char(service_handle,
                               &threshold_uuid,
                               ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
                               ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_WRITE,
                               &char_val,
                               NULL);

        char_val.attr_max_len = sizeof(ssid);
        char_val.attr_len = strlen(ssid);
        char_val.attr_value = (uint8_t *)ssid;

        esp_ble_gatts_add_char(service_handle,
                               &ssid_uid,
                               ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
                               ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_WRITE,
                               &char_val,
                               NULL);

        char_val.attr_max_len = sizeof(threshold);
        char_val.attr_len = sizeof(threshold);
        char_val.attr_value = (uint8_t *)&threshold;

        esp_ble_gatts_add_char(service_handle,
                               &wifi_password,
                               ESP_GATT_PERM_WRITE,
                               ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_WRITE,
                               &char_val,
                               NULL);

        char_val.attr_max_len = sizeof(rtc_epoch_time);
        char_val.attr_len = sizeof(rtc_epoch_time);
        char_val.attr_value = (uint8_t *)&rtc_epoch_time;

        esp_ble_gatts_add_char(service_handle,
                               &rtc_epoch_time_uuid,
                               ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
                               ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_WRITE,
                               &char_val,
                               NULL);

        ESP_LOGI(TAG, "Starting Advertising.");

        start_advertising();
        break;
    }

    default:
        break;
    }
}

esp_err_t ble_config_server_init(void)
{
    ESP_LOGI(TAG, "Initializing BLE");

    ESP_ERROR_CHECK(nvs_config_init());
    ESP_ERROR_CHECK(nvs_snvs_init());

    setenv("TZ", "UTC0", 1);
    tzset();

    esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT);

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_bt_controller_init(&bt_cfg));

    ESP_ERROR_CHECK(esp_bt_controller_enable(ESP_BT_MODE_BLE));

    ESP_ERROR_CHECK(esp_bluedroid_init());
    ESP_ERROR_CHECK(esp_bluedroid_enable());

    esp_ble_gatt_set_local_mtu(BLE_MAX_MTU);

    ESP_ERROR_CHECK(esp_ble_gatts_register_callback(gatts_event_handler));
    ESP_ERROR_CHECK(esp_ble_gatts_app_register(0));

    return ESP_OK;
}

esp_err_t setRtcTime(int epochSeconds)
{
    time_t epoch = 1772548570;

    struct timeval tv = {
        .tv_sec = epoch,
        .tv_usec = 0};

    settimeofday(&tv, NULL);

    time_t now;
    time(&now);

    struct tm utc_tm;
    gmtime_r(&now, &utc_tm);

    ESP_LOGI(TAG, "Current UTC time: %s", asctime(&utc_tm));

    return ESP_OK;
}