#include <stdio.h>
#include "esp_crt_bundle.h"
#include "esp_log.h"
#include "api_request_handler.h"


// Buffer to store response
#define MAX_HTTP_OUTPUT_BUFFER 2048
static char local_response_buffer[MAX_HTTP_OUTPUT_BUFFER] = {0};

// void api_get_remote_status(void){
//     memset(local_response_buffer, 0, MAX_HTTP_OUTPUT_BUFFER);
//     printf(" --- api_get_remote_status\n");

//     esp_http_client_config_t config = {
//         .url = "https://viber-bot-fastapi.vercel.app/walbot/",
//         // experimental (no verification | not secure)
//         .crt_bundle_attach = esp_crt_bundle_attach,   // use built-in CA certs,
         
//     };

//     esp_http_client_handle_t client = esp_http_client_init(&config);

//     esp_err_t err = esp_http_client_perform(client);

//     if (err == ESP_OK) {
//     int status_code = esp_http_client_get_status_code(client);
//     int content_len = esp_http_client_get_content_length(client);

//     // Only attempt to read if the status is good and there's content
//     if (status_code >= 200 && status_code < 300 && content_len > 0) {
        
//         // Use esp_http_client_read() to get the response data.
//         // It reads from the internal buffer/stream that esp_http_client_perform populated.
//         int read_len = esp_http_client_read(client, local_response_buffer, MAX_HTTP_OUTPUT_BUFFER - 1); // -1 for null terminator

//         if (read_len > 0) {
//             // Null-terminate the string safely, using the actual read length
//             local_response_buffer[read_len] = 0; 

//             printf("Content length: %d Read Length: %d\n", content_len, read_len);
//             printf("[%d] Response: %s\n", status_code, local_response_buffer);
//         } else {
//             printf("Content length: %d Read Length: %d\n", content_len, read_len);
//             printf("[%d] No content read or read error.\n", status_code);
//         }
//     } else {
//          printf("[%d] Request successful, but no content to read (Content Length: %d).\n", status_code, content_len);
//     }
// }
//     else{
//         ESP_LOGE(" API Handler", "HTTP GET request failed: %s", esp_err_to_name(err));

//     }

//     esp_http_client_cleanup(client);
// }


// Static variable to store the response, similar to your original setup
// static char local_response_buffer[MAX_HTTP_OUTPUT_BUFFER] = {0};
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