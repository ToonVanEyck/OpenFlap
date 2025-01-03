#include "oled_disp.h"
#include "driver/gpio.h"
// #include "driver/i2c_master.h"
// #include "esp_lcd_io_i2c.h"
// #include "esp_lcd_panel_io.h"
// #include "esp_lcd_panel_ops.h"
// #include "esp_lcd_panel_vendor.h"
#include "u8g2.h"
#include "u8g2_esp32_hal.h"

#define TAG "OLED_DISP"

#define I2C_MASTER_SCL_IO GPIO_NUM_26 /*!< GPIO number used for I2C master clock */
#define I2C_MASTER_SDA_IO GPIO_NUM_25 /*!< GPIO number used for I2C master data  */

esp_err_t oled_disp_init(void)
{

    u8g2_esp32_hal_t u8g2_esp32_hal = U8G2_ESP32_HAL_DEFAULT;
    u8g2_esp32_hal.bus.i2c.sda      = I2C_MASTER_SDA_IO;
    u8g2_esp32_hal.bus.i2c.scl      = I2C_MASTER_SCL_IO;
    u8g2_esp32_hal_init(u8g2_esp32_hal);

    u8g2_t u8g2; // a structure which will contain all the data for one display
    u8g2_Setup_ssd1306_i2c_128x32_univision_f(&u8g2, U8G2_R0,
                                              // u8x8_byte_sw_i2c,
                                              u8g2_esp32_i2c_byte_cb,
                                              u8g2_esp32_gpio_and_delay_cb); // init u8g2 structure
    u8x8_SetI2CAddress(&u8g2.u8x8, 0x78);

    u8g2_InitDisplay(&u8g2); // send init sequence to the display, display is in
                             // sleep mode after this,

    u8g2_SetPowerSave(&u8g2, 0); // wake up display
    u8g2_ClearBuffer(&u8g2);
    // u8g2_DrawBox(&u8g2, 0, 26, 80, 6);
    // u8g2_DrawFrame(&u8g2, 0, 26, 100, 6);
    u8g2_SetFont(&u8g2, u8g2_font_inb16_mf);
    u8g2_DrawStr(&u8g2, 1, 24, "OpenFlap");
    u8g2_SendBuffer(&u8g2);

    return ESP_OK;
}