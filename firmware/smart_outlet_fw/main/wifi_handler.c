#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "driver/gpio.h"
#include "esp_wifi.h"
#include "esp_wpa2.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_netif.h"
#include "esp_smartconfig.h"
#include "wifi_handler.h"

static const char *TAG = "WIFI_HANDLER";

WIFI_RUNTIME_DATA g_wifi_state_data = WIFI_RUNTIME_DEFAULT_DATA;

static EventGroupHandle_t s_wifi_event_group;
static uint8_t g_led_state = 0;
static uint32_t s_retry_num = 0;

static void ip_event_handler(int32_t event_id, void* event_data)
{
    ip_event_got_ip_t* event;

    if (event_id != IP_EVENT_STA_GOT_IP)
    {
        return;
    }

    event = (ip_event_got_ip_t*) event_data;

    ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
    s_retry_num = 0;
    xEventGroupSetBits(s_wifi_event_group, CONNECTED);
}

static void start_wifi_event_handler(int32_t event_id)
{
    if (event_id != WIFI_EVENT_STA_START)
    {
        return;
    }

    if(!g_wifi_state_data.is_configured)
    {
        xTaskCreate(smartconfig_polling_task, "smartconfig_polling_task", 2048, NULL, 5, NULL);
        g_wifi_state_data.connection_state = UNKNOWN;

        return;
    }

    esp_wifi_connect();
}

static void wifi_disconnect_event_handler(int32_t event_id)
{
    if (event_id != WIFI_EVENT_STA_DISCONNECTED)
    {
        return;
    }
    
    if (s_retry_num < WIFI_MAX_RETRY_COUNT)
    {
        esp_wifi_connect();
        s_retry_num++;
        g_wifi_state_data.connection_state = RETRY;

        return;
    }

    xEventGroupSetBits(s_wifi_event_group, NOT_CONNECTED);
}

static void smartconfig_event_handler(int32_t event_id, void* event_data)
{
    switch(event_id)
    {
        case SC_EVENT_SCAN_DONE:
            ESP_LOGI(TAG, "Scan done");
            break;
        case SC_EVENT_FOUND_CHANNEL:
            ESP_LOGI(TAG, "Found channel");
            break;
        case SC_EVENT_GOT_SSID_PSWD:
            ESP_LOGI(TAG, "Got SSID and password");

            smartconfig_event_got_ssid_pswd_t *evt = (smartconfig_event_got_ssid_pswd_t *)event_data;
  
            memcpy(g_wifi_state_data.ssid, evt->ssid, sizeof(g_wifi_state_data.ssid));
            memcpy(g_wifi_state_data.password, evt->password, sizeof(g_wifi_state_data.password));
            g_wifi_state_data.is_configured = 1;

            ESP_LOGI(TAG, "SSID: %s", evt->ssid);
            ESP_LOGI(TAG, "PASSWORD: %s", evt->password);

            ESP_ERROR_CHECK( esp_wifi_disconnect() );
            configure_wifi_credentials();
            esp_wifi_connect();
            break;
        case SC_EVENT_SEND_ACK_DONE:
            ESP_LOGI(TAG, "Send ack done");
            esp_smartconfig_stop();
            xEventGroupSetBits(s_wifi_event_group, ESPTOUCH_DONE);
            break;
    }
}

static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT)
    {
        start_wifi_event_handler(event_id);
        wifi_disconnect_event_handler(event_id);

        return;
    }

    if(event_base == IP_EVENT)
    {
        ip_event_handler(event_id, event_data);

        return;
    }

    if(event_base == SC_EVENT)
    {
        smartconfig_event_handler(event_id, event_data);

        return;
    }
}

static void configure_wifi_credentials(void)
{
    if (!g_wifi_state_data.is_configured)
    {
        return;
    }

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = "",
            .password = "",
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };

    strncpy((char *)wifi_config.sta.ssid, (char *)g_wifi_state_data.ssid, sizeof(wifi_config.sta.ssid));
    strncpy((char *)wifi_config.sta.password, (char *)g_wifi_state_data.password, sizeof(wifi_config.sta.password));

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
}

static void smartconfig_polling_task(void * parm)
{
    smartconfig_start_config_t cfg = SMARTCONFIG_START_CONFIG_DEFAULT();;

    ESP_ERROR_CHECK( esp_smartconfig_set_type(SC_TYPE_ESPTOUCH) );
    ESP_ERROR_CHECK( esp_smartconfig_start(&cfg) );

    while (1) 
    {
        if(g_wifi_state_data.connection_state == ESPTOUCH_DONE)
        {
            ESP_LOGI(TAG, "SmartConfig complete, exiting task...");
            g_wifi_state_data.connection_state = CONNECTED;
            esp_smartconfig_stop();
            vTaskDelete(NULL);
        }

        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

void initialize_wifi(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    s_wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
    assert(sta_netif);

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );

    ESP_ERROR_CHECK( esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL) );
    ESP_ERROR_CHECK( esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL) );
    ESP_ERROR_CHECK( esp_event_handler_register(SC_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL) );

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    configure_wifi_credentials();
    ESP_ERROR_CHECK(esp_wifi_start());
    config_status_led();

    g_wifi_state_data.connection_state = xEventGroupWaitBits(s_wifi_event_group,
            CONNECTED | NOT_CONNECTED,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);
}

void config_status_led()
{
    gpio_reset_pin(BLINK_GPIO);
    gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);
}

static void change_led_state_wifi()
{
    if(g_wifi_state_data.connection_state == CONNECTED)
    {
        gpio_set_level(BLINK_GPIO, 1);
    }
    else
    {
        g_led_state ^= 1;
        gpio_set_level(BLINK_GPIO, g_led_state);
    }
}

void poll_wifi_status()
{
    while(1)
    {
        change_led_state_wifi();
        vTaskDelay(200 / portTICK_PERIOD_MS);
    }
}