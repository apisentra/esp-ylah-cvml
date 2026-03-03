idf_component_register(
    SRCS "cv.c"
         "camera.c"
         "hello_world_main.c"
         "tasks/take_images.c"
         "tasks/write_sd.c"
    REQUIRES ble_config nvs_flash nvs_sec_provider
    PRIV_REQUIRES spi_flash fatfs
)