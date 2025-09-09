#include <stdio.h>
#include "server_handler.h"
#include "esp_log.h"
#include "raven_nvs_handler.h"

// ================= HTTP REQUEST HANDLERS =================
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
    ESP_LOGI("SERVER", "Received data: %s", buf);

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
    ESP_LOGI("SERVER", "Received data: %s", buf);

    // Reply back
    httpd_resp_send(req, "Data received", HTTPD_RESP_USE_STRLEN);
    nvs_memory_store("PASS", buf); // Store received data in NVS
                                   // Read back from NVS to verify
    return ESP_OK;
}


// ================= HTTP SERVER START =================
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

