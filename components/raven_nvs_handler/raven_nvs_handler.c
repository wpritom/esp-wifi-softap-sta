#include <stdio.h>
#include "raven_nvs_handler.h"

char read_nvs_buffer[100];
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
        // ESP_LOGE("NVS", "Error (%s) opening NVS handle!", esp_err_to_name(ret));
        return 0;
    }
    // else
    // {
    //     // ESP_LOGI("NVS", "NVS handle opened successfully");
    // }
    
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
        // ESP_LOGI("CLIENT", "The value is not initialized yet!");
        return 0;
    }
    else
    {
        ESP_LOGE("CLIENT", "Error (%s) reading from NVS!", esp_err_to_name(ret));
    }
    nvs_close(my_handle);
    return 0;
}

void nvs_memory_erase(const char *key)
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
    ret = nvs_erase_key(my_handle, key);
    if (ret != ESP_OK)
    {
        ESP_LOGE("NVS", "Error (%s) erasing from NVS!", esp_err_to_name(ret));
    }
    ret = nvs_commit(my_handle);
    if (ret != ESP_OK)
    {
        ESP_LOGE("NVS", "Error (%s) committing to NVS!", esp_err_to_name(ret));
    }
    nvs_close(my_handle);
}
