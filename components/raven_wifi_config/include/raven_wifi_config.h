#include "esp_mac.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include <stdint.h>
// #include "mdns.h"

#define EXAMPLE_ESP_WIFI_SSID "RAVEN"
#define EXAMPLE_ESP_WIFI_PASS "12345678"
#define EXAMPLE_ESP_WIFI_CHANNEL 1
#define EXAMPLE_MAX_STA_CONN 10



#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1
#define EXAMPLE_ESP_MAXIMUM_RETRY 3

extern char sta_ssid[32];
extern char sta_pass[32];
extern char device_mac_str[18];
// extern uint8_t WIFI_CONNECTED;
extern uint8_t WIFI_GLOBAL_INIT;


void event_handler_sta(void *arg, esp_event_base_t event_base,
                              int32_t event_id, void *event_data);

// void wifi_init_sta(void);

void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data);
// void wifi_init_softap(void);

// new api
void wifi_init_all(void);
void wifi_set_ap(void);
void wifi_set_sta();
uint8_t is_wifi_sta_connected(void);
void check_and_retry_wifi(uint8_t retry);