#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_mac.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"

#include "esp_netif.h"
#include "esp_http_server.h" // <-- Add this
// #include <event_groups.h>

/* The examples use WiFi configuration that you can set via project configuration menu.

   If you'd rather not, just change the below entries to strings with
   the config you want - ie #define EXAMPLE_WIFI_SSID "mywifissid"
*/
#define EXAMPLE_ESP_WIFI_SSID "RAVEN"
#define EXAMPLE_ESP_WIFI_PASS "12345678"
#define EXAMPLE_ESP_WIFI_CHANNEL 1
#define EXAMPLE_MAX_STA_CONN 2

static const char *TAG = "wifi softAP";

char read_nvs_buffer[100]; // Buffer to store the retrieved value

// ================= NVS HANDLERS =================

void nvs_memory_store(const char *key, const char *data)
{
    nvs_handle_t my_handle;
    esp_err_t ret = nvs_open("storage", NVS_READWRITE, &my_handle);
    if (ret != ESP_OK)
    {
        ESP_LOGE("NVS", "Error (%s) opening NVS handle!", esp_err_to_name(ret));
    }
    else
    {
        ESP_LOGI("NVS", "NVS handle opened successfully");
    }
    ret = nvs_set_str(my_handle, key, data);

    if (ret != ESP_OK)
    {
        ESP_LOGE("NVS", "Error (%s) writing to NVS!", esp_err_to_name(ret));
    }
    ret = nvs_commit(my_handle);
    if (ret != ESP_OK)
    {
        ESP_LOGE("NVS", "Error (%s) committing to NVS!", esp_err_to_name(ret));
    }
    nvs_close(my_handle);
}

uint8_t nvs_memory_read(const char *key)
{
    nvs_handle_t my_handle;
    esp_err_t ret = nvs_open("storage", NVS_READONLY, &my_handle);
    if (ret != ESP_OK)
    {
        ESP_LOGE("NVS", "Error (%s) opening NVS handle!", esp_err_to_name(ret));
    }
    else
    {
        ESP_LOGI("NVS", "NVS handle opened successfully");
    }
    char stored_value[50];                       // Buffer to store the retrieved value
    size_t required_size = sizeof(stored_value); // Size of the buffer

    ret = nvs_get_str(my_handle, key, stored_value, &required_size);

    if (ret == ESP_OK)
    {
        ESP_LOGI("CLIENT", "Stored value: KEY: %s VALUE: %s", key, stored_value);
        strcpy(read_nvs_buffer, stored_value);

        return 1;
    }
    else if (ret == ESP_ERR_NVS_NOT_FOUND)
    {
        ESP_LOGI("CLIENT", "The value is not initialized yet!");
    }
    else
    {
        ESP_LOGE("CLIENT", "Error (%s) reading from NVS!", esp_err_to_name(ret));
    }
    nvs_close(my_handle);
    return 0;
}

// ================= HTTP SERVER HANDLERS =================

esp_err_t hello_get_handler(httpd_req_t *req)
{

    const char *resp_str = "<h1>Hello from ESP32!</h1>";
    httpd_resp_send(req, resp_str, strlen(resp_str));
    nvs_memory_read("SSID");
    nvs_memory_read("PASS");
    return ESP_OK;
}

esp_err_t data_post_handler(httpd_req_t *req)
{
    char buf[100];
    int ret = httpd_req_recv(req, buf, sizeof(buf) - 1);
    if (ret <= 0)
    {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    buf[ret] = '\0'; // Null terminate
    ESP_LOGI(TAG, "Received data: %s", buf);

    // Reply back
    httpd_resp_send(req, "Data received", HTTPD_RESP_USE_STRLEN);
    nvs_memory_store("SSID", buf); // Store received data in NVS
                                   // Read back from NVS to verify
    return ESP_OK;
}

esp_err_t pass_post_handler(httpd_req_t *req)
{
    char buf[100];
    int ret = httpd_req_recv(req, buf, sizeof(buf) - 1);
    if (ret <= 0)
    {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    buf[ret] = '\0'; // Null terminate
    ESP_LOGI(TAG, "Received data: %s", buf);

    // Reply back
    httpd_resp_send(req, "Data received", HTTPD_RESP_USE_STRLEN);
    nvs_memory_store("PASS", buf); // Store received data in NVS
                                   // Read back from NVS to verify
    return ESP_OK;
}

httpd_handle_t start_webserver(void)
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    if (httpd_start(&server, &config) == ESP_OK)
    {
        // Register GET handler
        httpd_uri_t hello_uri = {
            .uri = "/hello",
            .method = HTTP_GET,
            .handler = hello_get_handler,
            .user_ctx = NULL};
        httpd_register_uri_handler(server, &hello_uri);

        // Register POST handler
        httpd_uri_t data_uri = {
            .uri = "/ssid",
            .method = HTTP_POST,
            .handler = data_post_handler,
            .user_ctx = NULL};
        httpd_register_uri_handler(server, &data_uri);

        httpd_uri_t pass_uri = {
            .uri = "/pass",
            .method = HTTP_POST,
            .handler = pass_post_handler,
            .user_ctx = NULL};
        httpd_register_uri_handler(server, &pass_uri);
    }
    return server;
}
// ================= WiFi SOFTAP HANDLERS =================
const char *TAG_STA = "wifi station";
/* FreeRTOS event group to signal when we are connected*/
EventGroupHandle_t s_wifi_event_group;

int s_retry_num = 0;
char sta_ssid[32];
char sta_pass[32];
/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1
#define EXAMPLE_ESP_MAXIMUM_RETRY 10

static void event_handler_sta(void *arg, esp_event_base_t event_base,
                              int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY)
        {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "retry to connect to the AP");
        }
        else
        {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(TAG, "connect to the AP fail");
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

void wifi_init_sta(void)
{
    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler_sta,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &event_handler_sta,
                                                        NULL,
                                                        &instance_got_ip));

    wifi_config_t wifi_config = {
        .sta = {
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
            .sae_pwe_h2e = WPA3_SAE_PWE_BOTH,
        },
    };
    // Copy SSID and password into the struct
    strncpy((char *)wifi_config.sta.ssid, sta_ssid, sizeof(wifi_config.sta.ssid));
    strncpy((char *)wifi_config.sta.password, sta_pass, sizeof(wifi_config.sta.password));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG_STA, "wifi_init_sta finished.");

    /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
     * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
                                           WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                           pdFALSE,
                                           pdFALSE,
                                           portMAX_DELAY);

    /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
     * happened. */
    if (bits & WIFI_CONNECTED_BIT)
    {
        ESP_LOGI(TAG_STA, "connected to ap SSID:%s password:%s",
                 EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
    }
    else if (bits & WIFI_FAIL_BIT)
    {
        ESP_LOGI(TAG_STA, "Failed to connect to SSID:%s, password:%s",
                 EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
    }
    else
    {
        ESP_LOGE(TAG_STA, "UNEXPECTED EVENT");
    }
}
// ================= WiFi SOFTAP =================

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    if (event_id == WIFI_EVENT_AP_STACONNECTED)
    {
        wifi_event_ap_staconnected_t *event = (wifi_event_ap_staconnected_t *)event_data;
        ESP_LOGI(TAG, "station " MACSTR " join, AID=%d",
                 MAC2STR(event->mac), event->aid);
    }
    else if (event_id == WIFI_EVENT_AP_STADISCONNECTED)
    {
        wifi_event_ap_stadisconnected_t *event = (wifi_event_ap_stadisconnected_t *)event_data;
        ESP_LOGI(TAG, "station " MACSTR " leave, AID=%d",
                 MAC2STR(event->mac), event->aid);
    }
}

void wifi_init_softap(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        NULL));

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = EXAMPLE_ESP_WIFI_SSID,
            .ssid_len = strlen(EXAMPLE_ESP_WIFI_SSID),
            .channel = EXAMPLE_ESP_WIFI_CHANNEL,
            .password = EXAMPLE_ESP_WIFI_PASS,
            .max_connection = EXAMPLE_MAX_STA_CONN,
            .authmode = WIFI_AUTH_WPA2_PSK,
            .pmf_cfg = {
                .required = true,
            },
        },
    };
    if (strlen(EXAMPLE_ESP_WIFI_PASS) == 0)
    {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_softap finished. SSID:%s password:%s channel:%d",
             EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS, EXAMPLE_ESP_WIFI_CHANNEL);
}

void app_main(void)
{
    uint8_t IS_CONFIGURED = 0;
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    // nvs_flash_erase();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // read wifi config from NVS
    // char sta_ssid[32];
    // char sta_pass[32];
    if (nvs_memory_read("SSID"))
    {
        strcpy(sta_ssid, read_nvs_buffer);
        IS_CONFIGURED++;
    }
    if (nvs_memory_read("PASS"))
    {
        strcpy(sta_pass, read_nvs_buffer);
        IS_CONFIGURED++;
    }

    if (IS_CONFIGURED == 2)
    {
        ESP_LOGI(TAG_STA, "SSID: %s, PASS: %s", sta_ssid, sta_pass);
        ESP_LOGI(TAG_STA, "ESP_WIFI_MODE_STA POSSIBLE");
        wifi_init_sta();
    }
    else
    {
        ESP_LOGI(TAG, "ESP_WIFI_MODE_STA IMPOSSIBLE");
        wifi_init_softap();
        start_webserver();
        ESP_LOGI(TAG, "ESP_WIFI_MODE_AP");
    }

    ///////////////////////////////////////////////////////////////////////
    while (1)
    {
        // __NOP(); // <-  Prevent WDT Reset
        vTaskDelay(1000 / portTICK_PERIOD_MS); // <- 1 Second
        printf("Main Loop\n");
    }
}