idf_component_register(
    SRCS "main.cpp"
    INCLUDE_DIRS "."
    REQUIRES esp_driver_gpio pid_ctrl bdc_motor
)
