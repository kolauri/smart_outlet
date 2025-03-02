#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "smart_outlet_main.h"
#include "wifi_handler.h"
#include "nvs_flash.h"

void app_main(void)
{
    ESP_ERROR_CHECK( nvs_flash_init() );
    initialize_wifi();

    xTaskCreate(poll_wifi_status, "wifi_status_task", 1024 * 2, NULL, configMAX_PRIORITIES - 5, NULL);
}