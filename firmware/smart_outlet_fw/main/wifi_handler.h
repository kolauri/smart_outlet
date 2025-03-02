#ifndef WIFI_HANDLER_H
#define WIFI_HANDLER_H

#define WIFI_MAX_RETRY_COUNT 5

#define BLINK_GPIO GPIO_NUM_2

#define WIFI_RUNTIME_DEFAULT_DATA { .ssid = "DEFAULT", \
                                    .password = "DEFAULT", \
                                    .connection_state = NOT_CONNECTED, \
                                    .is_configured = 0 }

typedef enum
{
    UNKNOWN,
    CONNECTED,
    NOT_CONNECTED,
    RETRY,
    ESPTOUCH_DONE
}WIFI_CONNECTION_E;

typedef struct
{
    uint8_t ssid[32];
    uint8_t password[64];
    WIFI_CONNECTION_E connection_state;
    uint8_t is_configured;
    
}WIFI_RUNTIME_DATA;

void initialize_wifi(void);
static void configure_wifi_credentials(void);
static void smartconfig_polling_task(void * parm);
void config_status_led();
void poll_wifi_status();

#endif