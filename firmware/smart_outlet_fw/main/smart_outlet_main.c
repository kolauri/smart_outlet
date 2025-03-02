#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "smart_outlet_main.h"
#include "wifi_handler.h"
#include "nvs_read_write.h"

extern SMART_OUTLET_CONF_T smart_outlet_conf_data;
extern WIFI_RUNTIME_DATA_T g_wifi_state_data;

void app_main(void)
{
    initialize_flash();
    initialize_wifi();

    xTaskCreate(poll_wifi_status, "wifi_status_task", 1024 * 2, NULL, configMAX_PRIORITIES - 5, NULL);
}