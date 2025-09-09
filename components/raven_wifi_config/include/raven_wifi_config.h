#include "esp_mac.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "driver/gpio.h"

#define EXAMPLE_ESP_WIFI_SSID "RAVEN"
#define EXAMPLE_ESP_WIFI_PASS "12345678"
#define EXAMPLE_ESP_WIFI_CHANNEL 1
#define EXAMPLE_MAX_STA_CONN 10


#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1
#define EXAMPLE_ESP_MAXIMUM_RETRY 10

extern char sta_ssid[32];
extern char sta_pass[32];
extern uint8_t WIFI_CONNECTED;

void event_handler_sta(void *arg, esp_event_base_t event_base,
                              int32_t event_id, void *event_data);

void wifi_init_sta(void);

void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data);
void wifi_init_softap(void);
