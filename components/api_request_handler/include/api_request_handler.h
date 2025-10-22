// #include "esp_http_client.h"
#include "raven_wifi_config.h"

#define HTTP_REQUEST_TIMEOUT_MS 10000  // 10 seconds
#define MAX_HTTP_OUTPUT_BUFFER 2048
#define MAX_POST_DATA 256

typedef struct {
    bool in_progress;
    bool done;
    bool failed;
    int status_code;
    char response[MAX_HTTP_OUTPUT_BUFFER];
    char post_data[MAX_POST_DATA];
    bool post_progress;
} http_result_t;

// Declare global result (defined in http_client.c)
extern http_result_t raven_http_result;



// void api_get_remote_status(void);
// void api_post_device_data(const char *device_id,
//                           const char *device_secret,
//                           const char *uuid,
//                           int dpid,
//                           int value);

// void api_post_device_pairing(const char *device_id,
//                              const char *device_pid,
//                              const char *device_secret);

void async_client_cleanup();
void async_api_get_device_pairing(const char *device_id,
                             const char *device_pid,
                             const char *device_secret);
// bool await_http_request(void);

void async_api_post_device_data(const char *device_id,
                          const char *device_secret,
                          const char *uuid,
                          int dpid,
                          int value);

// void await();

http_result_t get_raven_http_result(void);