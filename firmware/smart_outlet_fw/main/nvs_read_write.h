#ifndef NVS_READ_WRITE_H
#define NVS_READ_WRITE_H

#define STORAGE_NAMESPACE "storage"
#define KEY_NAME "conf"

#include "wifi_handler.h"

typedef struct
{
    WIFI_RUNTIME_DATA_T wifi_runtime_data;
}SMART_OUTLET_CONF_T;

esp_err_t initialize_flash();
esp_err_t get_conf_from_flash();
esp_err_t write_conf_to_flash();

#endif