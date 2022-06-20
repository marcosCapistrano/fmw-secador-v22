idf_component_register(SRCS "storage.c" "state_manager.c"
                    INCLUDE_DIRS "include"
                    REQUIRES nvs_flash common)
