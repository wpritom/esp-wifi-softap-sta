#include "nvs_flash.h"
#include "esp_log.h"
#include <string.h>

extern char read_nvs_buffer[100]; // Buffer to store the retrieved value

// ================= NVS HANDLERS =================
void nvs_memory_store(const char *key, const char *data);
uint8_t nvs_memory_read(const char *key);
void nvs_memory_erase(const char *key);
void nvs_memory_store_num(const char *key, const int data);
uint8_t nvs_memory_read_num(const char *key);
