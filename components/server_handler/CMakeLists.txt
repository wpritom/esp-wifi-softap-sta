idf_component_register(SRCS "server_handler.c"
                    INCLUDE_DIRS "include"
                    REQUIRES esp_http_server raven_nvs_handler json raven_wifi_config raven_peer_handler
                    )
