#include <stdio.h>
#include "esp_crt_bundle.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "api_request_handler.h"


static int buffer_idx = 0;
http_result_t raven_http_result;

//// Asynchronous event handler
esp_http_client_handle_t async_client = NULL;  // Global handle initialized to NULL
// static bool async_http_request_failed = false;
// static bool http_request_in_progress = false;
// static bool http_request_failed = false;
// static bool http_request_done = false;
static uint32_t http_request_start_time = 0;


void async_client_cleanup() {
    if(async_client != NULL){
        esp_http_client_close(async_client);
        esp_http_client_cleanup(async_client);
        async_client = NULL;
    }
}

// const char *get_http_response(void)
// {
//     http_request_done = false;  // Reset after read
//     return local_response_buffer;
// }

http_result_t get_raven_http_result(void){
    return raven_http_result;
}

bool await_http_request(void)
{
    /* 
    If the request is in progress, perform the request. 
    This should be called if a request is in progress.
    Should be called just once.
    */
    esp_err_t err = ESP_OK;

    if (raven_http_result.in_progress && async_client != NULL)
    {
        err = esp_http_client_perform(async_client);

        if (err == ESP_ERR_HTTP_EAGAIN)
        {
            // Still in progress — check timeout
            if (esp_log_timestamp() - http_request_start_time > HTTP_REQUEST_TIMEOUT_MS) {
                ESP_LOGE("HTTP BUSY", "Timeout reached, forcing cleanup");
                raven_http_result.in_progress = false;
                async_client_cleanup();
            }
            return raven_http_result.in_progress;
        }
        else if (err == ESP_OK) // <-- ADD THIS CHECK
        {
            // If perform returns ESP_OK, the request finished *successfully* and
            // the event handler should have cleared http_request_in_progress.
            // We still need to cleanup the client though.
            raven_http_result.in_progress = false;
            async_client_cleanup();
        }
        else // (err != ESP_OK)
        {
            raven_http_result.in_progress = false;
            raven_http_result.failed = true;
            async_client_cleanup();
        }
    }

    // Check if the request is finished (either by success/finish event or a hard error)
    if (!raven_http_result.in_progress && async_client != NULL)
    {
        // This handles cleanup for HTTP_EVENT_ON_FINISH/DISCONNECTED
        async_client_cleanup();
    }
    return raven_http_result.in_progress;
}

static esp_err_t __async_http_event_handler(esp_http_client_event_t *evt)
{
    switch (evt->event_id) {
        case HTTP_EVENT_ERROR:
            raven_http_result.failed = true;
            ESP_LOGE("HTTP HANDLER >>>", "HTTP_EVENT_ERROR");
            break;

        case HTTP_EVENT_ON_DATA:
            if (buffer_idx + evt->data_len < MAX_HTTP_OUTPUT_BUFFER) {
                memcpy(raven_http_result.response + buffer_idx, evt->data, evt->data_len);
                buffer_idx += evt->data_len;
            }
            break;

        case HTTP_EVENT_ON_FINISH:
            raven_http_result.in_progress = false;
            raven_http_result.done = true;
            raven_http_result.response[buffer_idx] = '\0';
            raven_http_result.status_code = esp_http_client_get_status_code(evt->client);
            printf("[%d] RESPONSE %s\n", raven_http_result.status_code, raven_http_result.response);
            break;

        case HTTP_EVENT_DISCONNECTED:
            raven_http_result.in_progress = false;
            break;

        default:
            break;
    }
    return ESP_OK;
}

void init_request_session(){

    if (raven_http_result.in_progress) {
    ESP_LOGW("API", "Previous request still in progress, skipping new init");
    return;
    }   
   // Reset buffer before new request
    http_request_start_time = esp_log_timestamp();
    memset(&raven_http_result, 0, sizeof(raven_http_result));  // Reset all fields
    raven_http_result.in_progress = true;
    buffer_idx = 0;
}


esp_http_client_config_t async_config = {
        .event_handler = __async_http_event_handler,
        .crt_bundle_attach = esp_crt_bundle_attach,
        .timeout_ms=5000,
        .is_async=1,
    };

// APIS


void async_api_get_device_pairing(const char *device_id,
                             const char *device_pid,
                             const char *device_secret)
{
    if (!device_id || !device_secret || !device_pid) {
        ESP_LOGE("API", "Invalid arguments");
        return;
    }
    // init request session 
    init_request_session();

    ESP_LOGI("API", "Posting device pairing...");

    char get_url[256];

    int len = snprintf(get_url, sizeof(get_url),
                       "https://api.goloklab.com/iot/device/pair/?device_id=%s&device_pid=%s&device_secret=%s",
                       device_id, device_pid, device_secret);

    
    // HTTP client config
    async_config.url = get_url;
    async_client = esp_http_client_init(&async_config);

    if (async_client == NULL) {
        ESP_LOGE("API", "Failed to init HTTP client");
        return;
    }

     // Set request type and headers
    esp_err_t err = esp_http_client_set_method(async_client, HTTP_METHOD_GET);
    
    if (err != ESP_OK) {
        ESP_LOGE("API", "Failed to set method: %s", esp_err_to_name(err));
        esp_http_client_cleanup(async_client);
        return;
    }

    err = esp_http_client_perform(async_client);
    
    if (err == ESP_ERR_HTTP_EAGAIN) {
        // ESP_LOGI("API", "Async HTTP request started");
        raven_http_result.in_progress = true;
        return;  // ✅ Return immediately — non-blocking
    } 
    else if (err != ESP_OK) {
        ESP_LOGE("API", "HTTP perform failed: %s", esp_err_to_name(err));
        async_client_cleanup();
        raven_http_result.in_progress = false;
        return;
    }

}


void async_api_post_device_data(const char *device_id,
                          const char *device_secret,
                          const char *uuid,
                          int dpid,
                          int value)
{


    if (!device_id || !device_secret || !uuid) {
        ESP_LOGE("API", "Invalid arguments");
        return;
    }

     // init request session 
    init_request_session();

    ESP_LOGI("API", "Posting device data...");

    // Build JSON string
    char post_data[256];
    int len = snprintf(post_data, sizeof(post_data),
             "{\"device_id\":\"%s\",\"device_secret\":\"%s\",\"uuid\":\"%s\",\"dpid\":%d,\"value\":%d}",
             device_id, device_secret, uuid, dpid, value);

    printf("%s\n", post_data);
    if (len < 0 || len >= sizeof(post_data)) {
        ESP_LOGE("API", "JSON payload too large");
        return;
    }


    // HTTP client config
    async_config.url = "https://api.goloklab.com/iot/device/report";
    async_client = esp_http_client_init(&async_config);

    if (async_client == NULL) {
        ESP_LOGE("API", "Failed to init HTTP client");
        return;
    }
     // Set request type and headers
    esp_err_t err = esp_http_client_set_method(async_client, HTTP_METHOD_POST);
     // set header and Attach the JSON body
    esp_http_client_set_header(async_client, "Content-Type", "application/json");
    esp_http_client_set_post_field(async_client, post_data, strlen(post_data));

    err = esp_http_client_perform(async_client);
    
    if (err == ESP_ERR_HTTP_EAGAIN) {
        // ESP_LOGI("API", "Async HTTP request started");
        raven_http_result.in_progress = true;
        return;  // ✅ Return immediately — non-blocking
    } 
    else if (err != ESP_OK) {
        ESP_LOGE("API", "HTTP perform failed: %s", esp_err_to_name(err));
        async_client_cleanup();
        raven_http_result.in_progress = false;
        return;
    }

}