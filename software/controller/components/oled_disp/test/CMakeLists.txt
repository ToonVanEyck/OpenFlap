idf_component_register(SRCS "test.c"
                       INCLUDE_DIRS .
                       PRIV_REQUIRES unity test_utils u8g2 esp_driver_gpio u8g2-hal-esp-idf oled_disp networking)