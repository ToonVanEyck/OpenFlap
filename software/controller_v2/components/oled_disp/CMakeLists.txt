idf_component_register(
    SRCS 
        "oled_disp.c"
    INCLUDE_DIRS 
        "include"
    REQUIRES 
        u8g2 esp_driver_gpio u8g2-hal-esp-idf
        # esp_lcd esp_driver_i2c
)
