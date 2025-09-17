#include "esp_http_server.h"
#include "cJSON.h"

esp_err_t hello_get_handler(httpd_req_t *req);
esp_err_t data_post_handler(httpd_req_t *req);
esp_err_t pass_post_handler(httpd_req_t *req);
httpd_handle_t start_webserver(void);