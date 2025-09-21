#include "esp_http_server.h"
#include "cJSON.h"

uint8_t http_read_connect_command_and_reset(void);
esp_err_t hello_get_handler(httpd_req_t *req);
esp_err_t data_post_handler(httpd_req_t *req);
esp_err_t pass_post_handler(httpd_req_t *req);
httpd_handle_t start_webserver(void);
void stop_webserver(void);