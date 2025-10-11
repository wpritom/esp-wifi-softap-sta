#include "esp_http_client.h"
#include "raven_wifi_config.h"

void api_get_remote_status(void);
void api_post_device_data(const char *device_id,
                          const char *device_secret,
                          const char *uuid,
                          int dpid,
                          int value);
                          