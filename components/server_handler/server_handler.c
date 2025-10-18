#include <stdio.h>
#include "server_handler.h"
#include "esp_log.h"
#include "raven_nvs_handler.h"
#include "raven_wifi_config.h"
#include "raven_peer_handler.h"

uint8_t HTTP_CONNECT_COMMAND_FLAG = 0;
httpd_handle_t server = NULL;
uint8_t http_read_connect_command_and_reset(void)
{

    if (HTTP_CONNECT_COMMAND_FLAG == 1)
    {
        HTTP_CONNECT_COMMAND_FLAG = 0;
        return 1;
    }
    else
    {
        return 0;
    }
}

// ================= HTTP REQUEST HANDLERS =================
esp_err_t hello_get_handler(httpd_req_t *req)
{

    // const char *resp_str = "<h1>Hello from ESP32!</h1>";
    // httpd_resp_send(req, resp_str, strlen(resp_str));
    // read memory 
    char get_ssid[100];
    char get_pass[100];

    nvs_memory_read("SSID");
    strncpy((char *)get_ssid, read_nvs_buffer, sizeof(get_ssid));
    

    nvs_memory_read("PASS");
    strncpy((char *)get_pass, read_nvs_buffer, sizeof(get_pass));

    ESP_LOGI("SERVER", "SSID: %s", get_ssid);
    ESP_LOGI("SERVER", "PASS: %s", get_pass);
    ESP_LOGI("SERVER", "MAC: %s", device_mac_str);
    ESP_LOGI("SERVER", "DS: %s", DEVICE_SECRET);
    //
    // Create a JSON object
    cJSON *root = cJSON_CreateObject();
    if (!root) {
        ESP_LOGE("JSON", "Failed to create JSON object");
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    // Add data
    cJSON_AddStringToObject(root, "ssid", get_ssid);
    cJSON_AddStringToObject(root, "pass", get_pass);
    cJSON_AddStringToObject(root, "did", device_mac_str);
    cJSON_AddStringToObject(root, "ds", DEVICE_SECRET);
    cJSON_AddStringToObject(root, "pid", DEVICE_PID);
   

    // Convert JSON object to string
    char *json_string = cJSON_Print(root);
    if (!json_string) {
        ESP_LOGE("JSON", "Failed to convert JSON object to string");
        cJSON_Delete(root);
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    // Set content type
    httpd_resp_set_type(req, "application/json");

    // Send the JSON string
    httpd_resp_send(req, json_string, HTTPD_RESP_USE_STRLEN);
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


esp_err_t http_get_restart_for_connect(httpd_req_t *req){
    httpd_resp_send(req, "connect command received", HTTPD_RESP_USE_STRLEN);
    printf(" --- connect command received. device will restart soon --- \n");
    HTTP_CONNECT_COMMAND_FLAG = 1;
    // esp_restart();
    return ESP_OK;

}
// ================= HTTP SERVER START =================
httpd_handle_t start_webserver(void)
{

    if (server != NULL) {
        httpd_stop(server);
        server = NULL;
    }
    
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

        httpd_uri_t restart_for_connect_uri = {
            .uri = "/connect",
            .method = HTTP_GET,
            .handler = http_get_restart_for_connect,
            .user_ctx = NULL};
        httpd_register_uri_handler(server, &restart_for_connect_uri);

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

void stop_webserver(void)
{
    if (server != NULL) {
        // Stop the httpd server
        httpd_stop(server);
        // After stopping, the handle is no longer valid for further use
        server = NULL; // It's good practice to set it back to NULL
    }
}