idf_component_register(SRCS "raven_wifi_config.c"
                    INCLUDE_DIRS "include"
                    REQUIRES esp_wifi nvs_flash esp_event esp_http_server freertos esp_event driver esp_netif)
