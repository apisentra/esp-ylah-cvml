idf_component_register(
    SRCS "ble_config_server.c"
    INCLUDE_DIRS "."
    REQUIRES bt esp_common config_repo
)