#include <stdio.h>
#include "esp_crt_bundle.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "api_request_handler.h"

static int buffer_idx = 0;
http_result_t raven_http_result;

static char async_post_buffer[256];
esp_http_client_handle_t async_client = NULL;
static uint32_t http_request_start_time = 0;

// Track the worker task handle
static TaskHandle_t http_worker_handle = NULL;

// Mutex to protect state transitions
static SemaphoreHandle_t http_mutex = NULL;

void init_http_mutex(void) {
    if (http_mutex == NULL) {
        http_mutex = xSemaphoreCreateMutex();
    }
}

void async_client_cleanup(void) {
    if (async_client != NULL) {
        esp_http_client_close(async_client);
        esp_http_client_cleanup(async_client);
        async_client = NULL;
    }
    buffer_idx = 0;
}

void init_request_session() {
    if (http_worker_handle != NULL) {
        ESP_LOGW("API", "Waiting for previous request to complete...");
        
        // Wait up to 2 seconds for the task to finish
        int wait_count = 0;
        while (http_worker_handle != NULL && wait_count < 200) {
            vTaskDelay(pdMS_TO_TICKS(10));
            wait_count++;
        }
        
        if (http_worker_handle != NULL) {
            ESP_LOGE("API", "Previous task didn't exit cleanly, forcing cleanup");
            // Task is stuck, force cleanup
            vTaskDelete(http_worker_handle);
            http_worker_handle = NULL;
        }
    }

    // Force cleanup
    async_client_cleanup();
    
    // Reset state
    http_request_start_time = esp_log_timestamp();
    memset(&raven_http_result, 0, sizeof(raven_http_result));
    buffer_idx = 0;
}

static esp_err_t __async_http_event_handler(esp_http_client_event_t *evt) {
    switch (evt->event_id) {
        case HTTP_EVENT_ERROR:
            raven_http_result.failed = true;
            break;

        case HTTP_EVENT_ON_DATA:
            if (buffer_idx + evt->data_len < MAX_HTTP_OUTPUT_BUFFER) {
                memcpy(raven_http_result.response + buffer_idx, evt->data, evt->data_len);
                buffer_idx += evt->data_len;
            } else {
                raven_http_result.failed = true;
            }
            break;

        case HTTP_EVENT_ON_FINISH:
            raven_http_result.in_progress = false;
            raven_http_result.done = true;
            raven_http_result.response[buffer_idx] = '\0';
            raven_http_result.status_code = esp_http_client_get_status_code(evt->client);
            break;

        case HTTP_EVENT_DISCONNECTED:
            raven_http_result.in_progress = false;
            break;

        default:
            break;
    }
    return ESP_OK;
}

void http_worker_task(void *pv) {
    while (raven_http_result.in_progress && async_client != NULL) {
        esp_err_t err = esp_http_client_perform(async_client);

        if (err == ESP_ERR_HTTP_EAGAIN) {
            vTaskDelay(pdMS_TO_TICKS(10));

            if (esp_log_timestamp() - http_request_start_time > HTTP_REQUEST_TIMEOUT_MS) {
                ESP_LOGE("HTTP BUSY", "Timeout reached, forcing cleanup");
                raven_http_result.failed = true;
                raven_http_result.in_progress = false;
                async_client_cleanup();
                break;
            }
            continue;
        }
        else if (err == ESP_OK) {
            raven_http_result.done = true;
            raven_http_result.failed = false;
            raven_http_result.status_code = esp_http_client_get_status_code(async_client);
            async_client_cleanup();
            break;
        }
        else {
            raven_http_result.failed = true;
            ESP_LOGE("HTTP WORKER", "HTTP perform failed: %s", esp_err_to_name(err));
            raven_http_result.in_progress = false;
            async_client_cleanup();
            break;
        }
    }

    http_worker_handle = NULL;
    vTaskDelete(NULL);
}

esp_http_client_config_t async_config = {
    .event_handler = __async_http_event_handler,
    .crt_bundle_attach = esp_crt_bundle_attach,
    .timeout_ms = 10000,
    .is_async = 1,
};

void async_api_get_device_pairing(const char *device_id,
                                   const char *device_pid,
                                   const char *device_secret) {
    if (!device_id || !device_secret || !device_pid) {
        ESP_LOGE("API", "Invalid arguments");
        return;
    }

    init_request_session();
    ESP_LOGI("API", "Posting device pairing...");

    char get_url[256];
    int len = snprintf(get_url, sizeof(get_url),
                       "https://api.goloklab.com/iot/device/pair/?device_id=%s&device_pid=%s&device_secret=%s",
                       device_id, device_pid, device_secret);

    async_config.url = get_url;
    async_client = esp_http_client_init(&async_config);

    if (async_client == NULL) {
        ESP_LOGE("API", "Failed to init HTTP client");
        return;
    }

    esp_err_t err = esp_http_client_set_method(async_client, HTTP_METHOD_GET);
    if (err != ESP_OK) {
        ESP_LOGE("API", "Failed to set method: %s", esp_err_to_name(err));
        esp_http_client_cleanup(async_client);
        return;
    }

    // Start the task and store its handle
    raven_http_result.in_progress = true;
    xTaskCreate(http_worker_task, "http_worker", 8192, NULL, 5, &http_worker_handle);
}

void async_api_post_device_data(const char *device_id,
                                 const char *device_secret,
                                 const char *uuid,
                                 int dpid,
                                 int value) {
    if (!device_id || !device_secret || !uuid) {
        ESP_LOGE("API", "Invalid arguments");
        return;
    }

    init_request_session();
    ESP_LOGI("API", "Posting device data...");

    int len = snprintf(async_post_buffer, sizeof(async_post_buffer),
                       "{\"device_id\":\"%s\",\"device_secret\":\"%s\",\"uuid\":\"%s\",\"dpid\":%d,\"value\":%d}",
                       device_id, device_secret, uuid, dpid, value);

    printf("%s\n", async_post_buffer);
    if (len < 0 || len >= sizeof(async_post_buffer)) {
        ESP_LOGE("API", "JSON payload too large");
        return;
    }

    async_config.url = "https://api.goloklab.com/iot/device/report";
    async_client = esp_http_client_init(&async_config);

    if (async_client == NULL) {
        ESP_LOGE("API", "Failed to init HTTP client");
        return;
    }

    esp_err_t err = esp_http_client_set_method(async_client, HTTP_METHOD_POST);
    esp_http_client_set_header(async_client, "Content-Type", "application/json");
    esp_http_client_set_post_field(async_client, async_post_buffer, strlen(async_post_buffer));

    // Start the task and store its handle
    raven_http_result.in_progress = true;
    xTaskCreate(http_worker_task, "http_worker", 8192, NULL, 5, &http_worker_handle);
}