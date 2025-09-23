idf_component_register(SRCS "raven_peer_handler.c"
                    INCLUDE_DIRS "include"
                    REQUIRES raven_nvs_handler raven_wifi_config server_handler esp_wifi nvs_flash esp_event esp_http_server freertos esp_event esp_netif
                    )
