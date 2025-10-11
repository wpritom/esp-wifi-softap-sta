#include <stdio.h>
#include "esp_crt_bundle.h"
#include "esp_log.h"
#include "api_request_handler.h"


// Buffer to store response
#define MAX_HTTP_OUTPUT_BUFFER 2048
static char local_response_buffer[MAX_HTTP_OUTPUT_BUFFER] = {0};

static int buffer_idx = 0;

esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
    switch(evt->event_id) {
        // ... (other events like headers, error, finish) ...
        case HTTP_EVENT_ON_DATA:
            // Check for buffer overflow
            if (buffer_idx + evt->data_len < MAX_HTTP_OUTPUT_BUFFER) {
                memcpy(local_response_buffer + buffer_idx, evt->data, evt->data_len);
                buffer_idx += evt->data_len;
            } else {
                ESP_LOGE("HTTP_CLIENT", "Buffer overflow, dropping data!");
                // Optionally handle the error here
            }
            break;
        case HTTP_EVENT_DISCONNECTED:
            // Null-terminate the string on disconnection/end of body
            if (buffer_idx < MAX_HTTP_OUTPUT_BUFFER) {
                local_response_buffer[buffer_idx] = '\0';
            }
            break;
        default:
            break;
    }
    return ESP_OK;
}

void api_get_remote_status(void){
    // Reset buffer and index before new request
    memset(local_response_buffer, 0, MAX_HTTP_OUTPUT_BUFFER);
    buffer_idx = 0;
    printf(" --- api_get_remote_status\n");

    esp_http_client_config_t config = {
        .url = "https://api.goloklab.com/status/",
        .event_handler = _http_event_handler, // <-- ADD THE HANDLER HERE
        .crt_bundle_attach = esp_crt_bundle_attach,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_err_t err = esp_http_client_perform(client);

    if (err == ESP_OK) {
        int status_code = esp_http_client_get_status_code(client);
        
        // The event handler has already populated the buffer
        printf("[%d] Response: %s\n", status_code, local_response_buffer);
    }

    else{
        ESP_LOGE(" API Handler", "HTTP GET request failed: %s", esp_err_to_name(err));

    }
    
    esp_http_client_cleanup(client);
}


void api_post_device_data(const char *device_id,
                          const char *device_secret,
                          const char *uuid,
                          int dpid,
                          int value)
{
    // Reset buffer before new request
    memset(local_response_buffer, 0, MAX_HTTP_OUTPUT_BUFFER);
    buffer_idx = 0;

    ESP_LOGI("API", "Posting device data...");

    // Build JSON string
    char post_data[256];
    snprintf(post_data, sizeof(post_data),
             "{\"device_id\":\"%s\",\"device_secret\":\"%s\",\"uuid\":\"%s\",\"dpid\":%d,\"value\":%d}",
             device_id, device_secret, uuid, dpid, value);

    // HTTP client config
    esp_http_client_config_t config = {
        .url = "https://api.goloklab.com/iot/device/report",  // <-- Replace with your real endpoint
        .event_handler = _http_event_handler,
        .crt_bundle_attach = esp_crt_bundle_attach,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);

    // Set request type and headers
    esp_http_client_set_method(client, HTTP_METHOD_POST);
    esp_http_client_set_header(client, "Content-Type", "application/json");

    // Attach the JSON body
    esp_http_client_set_post_field(client, post_data, strlen(post_data));

    // Perform the request
    esp_err_t err = esp_http_client_perform(client);

    if (err == ESP_OK) {
        int status_code = esp_http_client_get_status_code(client);
        ESP_LOGI("API", "POST success, status = %d", status_code);
        ESP_LOGI("API", "Response: %s", local_response_buffer);
    } else {
        ESP_LOGE("API", "POST request failed: %s", esp_err_to_name(err));
    }

    // Cleanup
    esp_http_client_cleanup(client);
}
