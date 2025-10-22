#include <stdint.h>
#include "raven_nvs_handler.h"
#include "raven_wifi_config.h"
#include "server_handler.h"
#include "cJSON.h"

#define DEVICE_PID "68e35ff49b1c4a01b6b68975"
#define DEVICE_SECRET "s3cr3tK3y"


extern uint8_t RAVEN_AP_MODE;
extern uint8_t RAVEN_STA_MODE;

extern uint8_t connected_mode;
extern uint8_t wifi_mode;

void erase_wifi_config(void);
void device_softap_mode_init(void);
void device_station_mode_init(void);
uint8_t device_wifi_provision(void);

uint8_t isPaired(void);
void set_device_paired(void);
bool parse_uuid_save(const char *response);