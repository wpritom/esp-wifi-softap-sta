#include <stdio.h>
#include "esp_crt_bundle.h"
#include "esp_log.h"
#include "api_request_handler.h"


// Buffer to store response
#define MAX_HTTP_OUTPUT_BUFFER 2048
static char local_response_buffer[MAX_HTTP_OUTPUT_BUFFER] = {0};

void api_get_remote_status(void){
    printf(" --- api_get_remote_status\n");

    esp_http_client_config_t config = {
        .url = "https://viber-bot-fastapi.vercel.app/walbot/",
        // experimental (no verification | not secure)
        .crt_bundle_attach = esp_crt_bundle_attach,   // use built-in CA certs,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);

    esp_err_t err = esp_http_client_perform(client);

    if (err == ESP_OK) {
        int content_len = esp_http_client_get_content_length(client);
        int read_len = esp_http_client_read_response(client, local_response_buffer, MAX_HTTP_OUTPUT_BUFFER);
        int status_code = esp_http_client_get_status_code(client);
        
        printf("Content length: %d Read Length: %d\n", content_len, read_len);
        printf("[%d] Response: %s\n", status_code, local_response_buffer);
    }
    else{
        ESP_LOGE(" API Handler", "HTTP GET request failed: %s", esp_err_to_name(err));

    }

    esp_http_client_cleanup(client);
}