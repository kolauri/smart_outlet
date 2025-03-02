#include <stdio.h>
#include <inttypes.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "esp_log.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "driver/gpio.h"
#include "nvs_read_write.h"
#include "wifi_handler.h"

static const char *NVS_TAG = "NVS";

nvs_handle_t my_handle;

extern WIFI_RUNTIME_DATA_T g_wifi_state_data;

esp_err_t initialize_flash()
{
    esp_err_t err;

    err = nvs_flash_init();

    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) 
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK( err );

    err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &my_handle);

    if(err != ESP_OK)
    {
        ESP_LOGI(NVS_TAG, "Error initializing flash");
    }

    return err;
}

esp_err_t get_conf_from_flash()
{
    SMART_OUTLET_CONF_T smart_outlet_conf_data;
    esp_err_t err;
    size_t required_size = sizeof(SMART_OUTLET_CONF_T);

    err = nvs_get_blob(my_handle, KEY_NAME, &smart_outlet_conf_data, &required_size);

    if(err != ESP_OK)
    {
        ESP_LOGE(NVS_TAG, "Error getting conf data from NVS: %d", err);
        return err;
    }

    memcpy(&g_wifi_state_data, &smart_outlet_conf_data.wifi_runtime_data, sizeof(WIFI_RUNTIME_DATA_T));

    return ESP_OK;
}

esp_err_t write_conf_to_flash()
{
    SMART_OUTLET_CONF_T smart_outlet_conf_data;
    esp_err_t err;

    memcpy(&smart_outlet_conf_data.wifi_runtime_data, &g_wifi_state_data, sizeof(WIFI_RUNTIME_DATA_T));

    err = nvs_set_blob(my_handle, KEY_NAME, &smart_outlet_conf_data, sizeof(SMART_OUTLET_CONF_T));

    if (err != ESP_OK) {
        ESP_LOGE(NVS_TAG, "Error writing conf to NVS: %d", err);
        return err;
    }

    err = nvs_commit(my_handle);
    if (err != ESP_OK)
    {
        return err;
    }

    return ESP_OK;
}