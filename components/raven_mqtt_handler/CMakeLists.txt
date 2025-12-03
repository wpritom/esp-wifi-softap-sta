idf_component_register(SRCS "raven_mqtt_handler.c"
                    INCLUDE_DIRS "include"
                    REQUIRES mqtt
                    EMBED_TXTFILES certs/hivemq_ca.pem
                    )
