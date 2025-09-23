idf_component_register(SRCS "api_request_handler.c"
                    INCLUDE_DIRS "include"
                    REQUIRES esp_http_client raven_wifi_config mbedtls
                    )
