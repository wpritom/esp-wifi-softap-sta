
#include <stdint.h>
#include "raven_nvs_handler.h"
#include "raven_wifi_config.h"
#include "server_handler.h"

extern uint8_t RAVEN_AP_MODE;
extern uint8_t RAVEN_STA_MODE;

void erase_wifi_config(void);
void device_softap_mode_init(void);
void device_station_mode_init(void);
uint8_t device_wifi_provision(void);