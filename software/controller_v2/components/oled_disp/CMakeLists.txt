idf_component_register(
    SRCS "oled_disp.c"
    INCLUDE_DIRS "include"
    PRIV_REQUIRES u8g2 esp_driver_gpio u8g2-hal-esp-idf
        # esp_lcd esp_driver_i2c
)

# Check if QEMU is defined and add the define to the compiler flags
if(QEMU)
    target_compile_definitions(${COMPONENT_TARGET} PUBLIC "QEMU=1")
endif()