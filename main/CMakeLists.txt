idf_component_register(
    SRCS "main.cpp"
    INCLUDE_DIRS "."
    REQUIRES
        esp_driver_gpio esp_driver_pcnt esp_timer
        pid_ctrl bdc_motor 
)
