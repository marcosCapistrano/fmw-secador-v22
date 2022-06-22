idf_component_register(SRCS "server_manager.c"
                    INCLUDE_DIRS "include"
                    REQUIRES esp_http_server common mdns state_manager)
