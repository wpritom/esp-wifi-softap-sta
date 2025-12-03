idf_component_register(SRCS "raven_mqtt_handler.c"
                    INCLUDE_DIRS "include"
                    REQUIRES mqtt raven_peer_handler
                    EMBED_TXTFILES certs/hivemq_ca.pem
                    )
