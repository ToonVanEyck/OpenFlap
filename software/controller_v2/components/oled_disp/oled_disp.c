#include "oled_disp.h"
#include "driver/gpio.h"
#include "esp_check.h"
#include "esp_log.h"
#include <string.h>

#define TAG "OLED_DISP"

#if QEMU
#define QEMU_RETURN(_r) return _r
#else
#define QEMU_RETURN(_r) ;
#endif

#define I2C_MASTER_SCL_IO      GPIO_NUM_26 /*!< GPIO number used for I2C master clock */
#define I2C_MASTER_SDA_IO      GPIO_NUM_25 /*!< GPIO number used for I2C master data  */
#define I2C_SLAVE_SSD1306_ADDR 0x3C        /*!< slave address for SSD1306 display */
#define OLED_DISP_TASK_SIZE    2048        /*!< OLED display task stack size */

static void oled_dips_task(void *arg);
static void oled_disp_home_screen(oled_disp_ctx_t *ctx, const oled_disp_msg_home_t *msg);
static void oled_display_loading_screen(oled_disp_ctx_t *ctx, const oled_disp_msg_loading_t *msg);
static void oled_display_wifi_mode_screen(oled_disp_ctx_t *ctx, const oled_disp_msg_wifi_mode_t *msg);
static void oled_display_wifi_ap_screen(oled_disp_ctx_t *ctx, const oled_disp_msg_wifi_ap_t *msg);
static void oled_display_wifi_sta_screen(oled_disp_ctx_t *ctx, const oled_disp_msg_wifi_sta_t *msg);

//---------------------------------------------------------------------------------------------------------------------

esp_err_t oled_disp_init(oled_disp_ctx_t *ctx)
{
    QEMU_RETURN(ESP_OK); /* We can't use this module in qemu. */

    /* Clear the context. */
    memset(ctx, 0, sizeof(oled_disp_ctx_t));

    /* Initialize the u8g2 esp32 HAL. */
    ctx->u8g2_esp32_hal.bus.i2c.sda = I2C_MASTER_SDA_IO;
    ctx->u8g2_esp32_hal.bus.i2c.scl = I2C_MASTER_SCL_IO;
    u8g2_esp32_hal_init(ctx->u8g2_esp32_hal);

    /* Setup the ssd1306 128x32 OLD display driver. */
    u8g2_Setup_ssd1306_i2c_128x32_univision_f(&ctx->u8g2, U8G2_R0, u8g2_esp32_i2c_byte_cb,
                                              u8g2_esp32_gpio_and_delay_cb);

    /* COnfigure the I2C address used by the display. */
    u8x8_SetI2CAddress(&ctx->u8g2.u8x8, (I2C_SLAVE_SSD1306_ADDR << 1));

    /* Initialize the display. */
    u8g2_InitDisplay(&ctx->u8g2);
    /* Wake up the display. */
    u8g2_SetPowerSave(&ctx->u8g2, 0);

    /* Clear screen. */
    u8g2_ClearBuffer(&ctx->u8g2);
    u8g2_SendBuffer(&ctx->u8g2);
    u8g2_SetBitmapMode(&ctx->u8g2, 1);
    u8g2_SetFontMode(&ctx->u8g2, 1);

    /* Create the queue. */
    ctx->queue = xQueueCreate(5, sizeof(oled_disp_msg_t));
    ESP_RETURN_ON_FALSE(ctx->queue != NULL, ESP_FAIL, TAG, "Failed to create oled disp queue");

    /* Start the task. */
    ESP_RETURN_ON_FALSE(xTaskCreate(oled_dips_task, "oled_disp_task", OLED_DISP_TASK_SIZE, ctx, 10, &ctx->task),
                        ESP_FAIL, TAG, "Failed to create oled disp task");
    return ESP_OK;
}

//---------------------------------------------------------------------------------------------------------------------

esp_err_t oled_disp_clean(oled_disp_ctx_t *ctx)
{
    QEMU_RETURN(ESP_OK); /* We can't use this module in qemu. */

    /* Cleanup the queue. */
    vQueueDelete(ctx->queue);

    /* Stop the task. */
    vTaskDelete(ctx->task);

    /* Clear screen. */
    u8g2_ClearBuffer(&ctx->u8g2);
    u8g2_SendBuffer(&ctx->u8g2);

    /* Cleanup the display. */
    i2c_driver_delete(I2C_MASTER_NUM);

    return ESP_OK;
}

//---------------------------------------------------------------------------------------------------------------------

static void oled_dips_task(void *arg)
{
    oled_disp_ctx_t *ctx = (oled_disp_ctx_t *)arg;

    ESP_LOGI(TAG, "OLED display task started");

    while (1) {
        oled_disp_msg_t msg = {0};
        if (xQueueReceive(ctx->queue, &msg, portMAX_DELAY) == pdTRUE) {
            switch (msg.type) {
                case OLED_DISP_MSG_HOME_SCREEN:
                    oled_disp_home_screen(ctx, &msg.home);
                    break;
                case OLED_DISP_MSG_LOADING_SCREEN:
                    oled_display_loading_screen(ctx, &msg.loading);
                    break;
                case OLED_DISP_MEG_WIFI_MODE_SCREEN:
                    oled_display_wifi_mode_screen(ctx, &msg.wifi_mode);
                    break;
                case OLED_DISP_MSG_AP_SCREEN:
                    oled_display_wifi_ap_screen(ctx, &msg.wifi_ap);
                    break;
                case OLED_DISP_MSG_STA_SCREEN:
                    oled_display_wifi_sta_screen(ctx, &msg.wifi_sta);
                    break;
                default:
                    break;
            }
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------

esp_err_t oled_disp_home(oled_disp_ctx_t *ctx, const char *title, uint8_t version_major, uint8_t version_minor,
                         uint8_t version_patch)
{
    QEMU_RETURN(ESP_OK); /* We can't use this module in qemu. */

    oled_disp_msg_t msg = {
        .type = OLED_DISP_MSG_HOME_SCREEN,
        .home =
            {
                .version_major = version_major,
                .version_minor = version_minor,
                .version_patch = version_patch,
            },
    };

    strncpy(msg.home.title, title, sizeof(msg.home.title) - 1);

    return xQueueSendToBack(ctx->queue, &msg, 0) == pdTRUE ? ESP_OK : ESP_FAIL;
};

//---------------------------------------------------------------------------------------------------------------------

esp_err_t oled_disp_loading(oled_disp_ctx_t *ctx, const char *title, uint8_t progress)
{
    QEMU_RETURN(ESP_OK); /* We can't use this module in qemu. */

    oled_disp_msg_t msg = {
        .type = OLED_DISP_MSG_LOADING_SCREEN,
        .loading =
            {
                .title    = {0},
                .progress = progress,
            },
    };

    strncpy(msg.loading.title, title, sizeof(msg.loading.title) - 1);

    return xQueueSendToBack(ctx->queue, &msg, 0) == pdTRUE ? ESP_OK : ESP_FAIL;
}

esp_err_t oled_disp_wifi_mode(oled_disp_ctx_t *ctx, bool wifi_ap_mode, bool wifi_sta_mode)
{
    QEMU_RETURN(ESP_OK); /* We can't use this module in qemu. */

    oled_disp_msg_t msg = {
        .type = OLED_DISP_MEG_WIFI_MODE_SCREEN,
        .wifi_mode =
            {
                .wifi_ap_mode  = wifi_ap_mode,
                .wifi_sta_mode = wifi_sta_mode,
            },
    };

    return xQueueSendToBack(ctx->queue, &msg, 0) == pdTRUE ? ESP_OK : ESP_FAIL;
}

//---------------------------------------------------------------------------------------------------------------------

esp_err_t oled_disp_wifi_ap(oled_disp_ctx_t *ctx, const char *ssid, uint8_t client_cnt)
{
    QEMU_RETURN(ESP_OK); /* We can't use this module in qemu. */

    oled_disp_msg_t msg = {
        .type = OLED_DISP_MSG_AP_SCREEN,
        .wifi_ap =
            {
                .client_cnt = client_cnt,
            },
    };

    strncpy(msg.wifi_ap.ssid, ssid, sizeof(msg.wifi_ap.ssid) - 1);

    return xQueueSendToBack(ctx->queue, &msg, 0) == pdTRUE ? ESP_OK : ESP_FAIL;
}

//---------------------------------------------------------------------------------------------------------------------

esp_err_t oled_disp_wifi_sta(oled_disp_ctx_t *ctx, const char *ssid, bool connected, const char *ip)
{
    QEMU_RETURN(ESP_OK); /* We can't use this module in qemu. */

    oled_disp_msg_t msg = {
        .type = OLED_DISP_MSG_STA_SCREEN,
        .wifi_sta =
            {
                .connected = connected,
            },
    };

    strncpy(msg.wifi_sta.ssid, ssid, sizeof(msg.wifi_sta.ssid) - 1);
    strncpy(msg.wifi_sta.ip, ip, sizeof(msg.wifi_sta.ip) - 1);

    return xQueueSendToBack(ctx->queue, &msg, 0) == pdTRUE ? ESP_OK : ESP_FAIL;
}

//---------------------------------------------------------------------------------------------------------------------

static void oled_disp_home_screen(oled_disp_ctx_t *ctx, const oled_disp_msg_home_t *msg)
{
    /* Prepare. */
    u8g2_ClearBuffer(&ctx->u8g2);
    u8g2_SetBitmapMode(&ctx->u8g2, 1);
    u8g2_SetFontMode(&ctx->u8g2, 1);

    /* Draw the title. */
    u8g2_SetFont(&ctx->u8g2, u8g2_font_profont29_tr);
    u8g2_DrawStr(&ctx->u8g2, 0, 19, msg->title);

    u8g2_SetFont(&ctx->u8g2, u8g2_font_profont12_tr);
    char version_str[16];
    snprintf(version_str, sizeof(version_str), "v%d.%d.%d", msg->version_major, msg->version_minor, msg->version_patch);
    u8g2_DrawStr(&ctx->u8g2, 0, 32, version_str);

    /* Send the buffer. */
    u8g2_SendBuffer(&ctx->u8g2);
}

//---------------------------------------------------------------------------------------------------------------------

static void oled_display_loading_screen(oled_disp_ctx_t *ctx, const oled_disp_msg_loading_t *msg)
{
    static const uint8_t pct_glyph_bits[] = {0x02, 0x25, 0x12, 0x08, 0x04, 0x12, 0x29, 0x10};

    /* Prepare. */
    u8g2_ClearBuffer(&ctx->u8g2);
    u8g2_SetBitmapMode(&ctx->u8g2, 1);
    u8g2_SetFontMode(&ctx->u8g2, 1);

    /* Draw title. */
    u8g2_SetFont(&ctx->u8g2, u8g2_font_profont17_tr); /* Max 11 chars? */
    u8g2_DrawStr(&ctx->u8g2, 0, 11, msg->title);

    /* Draw progress number. */
    u8g2_SetFont(&ctx->u8g2, u8g2_font_profont15_tr);
    char progress_str[4];
    snprintf(progress_str, sizeof(progress_str), "%3d", msg->progress);
    u8g2_DrawStr(&ctx->u8g2, 100, 11, progress_str);

    /* Draw "%" sign. */
    u8g2_SetDrawColor(&ctx->u8g2, 1);
    u8g2_DrawXBM(&ctx->u8g2, 122, 3, 6, 8, pct_glyph_bits);

    /* Draw loading bar. */
    u8g2_DrawFrame(&ctx->u8g2, 0, 16, 128, 15);
    u8g2_SetDrawColor(&ctx->u8g2, 2);
    if (msg->progress > 0) {
        u8g2_DrawBox(&ctx->u8g2, 3, 19, msg->progress * 122 / 100, 9);
    }

    /* Send the buffer. */
    u8g2_SendBuffer(&ctx->u8g2);
}

//---------------------------------------------------------------------------------------------------------------------

static void oled_display_wifi_mode_screen(oled_disp_ctx_t *ctx, const oled_disp_msg_wifi_mode_t *msg)
{
    /* Prepare. */
    u8g2_ClearBuffer(&ctx->u8g2);
    u8g2_SetBitmapMode(&ctx->u8g2, 1);
    u8g2_SetFontMode(&ctx->u8g2, 1);

    u8g2_SetFont(&ctx->u8g2, u8g2_font_profont17_tr);
    u8g2_DrawStr(&ctx->u8g2, 0, 11, "WiFi mode:");

    u8g2_SetFont(&ctx->u8g2, u8g2_font_profont15_tr);
    if (msg->wifi_ap_mode && msg->wifi_sta_mode) {
        u8g2_DrawStr(&ctx->u8g2, 0, 31, "AP & STA");
    } else if (msg->wifi_ap_mode) {
        u8g2_DrawStr(&ctx->u8g2, 0, 31, "AP");
    } else if (msg->wifi_sta_mode) {
        u8g2_DrawStr(&ctx->u8g2, 0, 31, "STA");
    } else {
        u8g2_DrawStr(&ctx->u8g2, 0, 31, "OFF");
    }

    /* Send the buffer. */
    u8g2_SendBuffer(&ctx->u8g2);
}

//---------------------------------------------------------------------------------------------------------------------

static void oled_display_wifi_ap_screen(oled_disp_ctx_t *ctx, const oled_disp_msg_wifi_ap_t *msg)
{
    static const uint8_t elide_glyph_bits[] = {0x15};

    /* Prepare. */
    u8g2_ClearBuffer(&ctx->u8g2);
    u8g2_SetBitmapMode(&ctx->u8g2, 1);
    u8g2_SetFontMode(&ctx->u8g2, 1);

    /* Draw title. */
    u8g2_SetFont(&ctx->u8g2, u8g2_font_profont17_tr);
    u8g2_DrawStr(&ctx->u8g2, 0, 11, "WiFi AP:");

    /* Draw ssid, elide if needed. */
    u8g2_SetFont(&ctx->u8g2, u8g2_font_profont11_tr);

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-truncation"
    static const uint8_t max_ssid_str_len = 20; /* A maximum of 20 characters can be displayed with font size 11.*/
    char ssid_str[max_ssid_str_len + 2];
    snprintf(ssid_str, sizeof(ssid_str), "ssid   : %s", msg->ssid);
    if (strlen(ssid_str) > max_ssid_str_len) {
        ssid_str[max_ssid_str_len] = '\0';
        u8g2_DrawXBM(&ctx->u8g2, 121, 20, 5, 1, elide_glyph_bits);
    }
    u8g2_DrawStr(&ctx->u8g2, 0, 20, ssid_str);

    char client_cnt_str[max_ssid_str_len + 1];
    snprintf(client_cnt_str, sizeof(client_cnt_str), "clients: %d", msg->client_cnt);
    u8g2_DrawStr(&ctx->u8g2, 0, 30, client_cnt_str);
#pragma GCC diagnostic pop
    /* Send the buffer. */
    u8g2_SendBuffer(&ctx->u8g2);
}

//---------------------------------------------------------------------------------------------------------------------

static void oled_display_wifi_sta_screen(oled_disp_ctx_t *ctx, const oled_disp_msg_wifi_sta_t *msg)
{
    static const uint8_t elide_glyph_bits[] = {0x15};

    /* Prepare. */
    u8g2_ClearBuffer(&ctx->u8g2);
    u8g2_SetBitmapMode(&ctx->u8g2, 1);
    u8g2_SetFontMode(&ctx->u8g2, 1);

    /* Draw title. */
    u8g2_SetFont(&ctx->u8g2, u8g2_font_profont17_tr);
    u8g2_DrawStr(&ctx->u8g2, 0, 11, "WiFi STA:");

    /* Draw ssid, elide if needed. */
    u8g2_SetFont(&ctx->u8g2, u8g2_font_profont11_tr);

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-truncation"
    static const uint8_t max_ssid_str_len = 20; /* A maximum of 20 characters can be displayed with font size 11.*/
    char ssid_str[max_ssid_str_len + 2];
    snprintf(ssid_str, sizeof(ssid_str), "ssid: %s", msg->ssid);
    if (strlen(ssid_str) > max_ssid_str_len) {
        ssid_str[max_ssid_str_len] = '\0';
        u8g2_DrawXBM(&ctx->u8g2, 121, 20, 5, 1, elide_glyph_bits);
    }
    u8g2_DrawStr(&ctx->u8g2, 0, 20, ssid_str);

    char ip_str[max_ssid_str_len + 1];
    snprintf(ip_str, sizeof(ip_str), "ip  : %s", msg->ip);
    u8g2_DrawStr(&ctx->u8g2, 0, 30, ip_str);
#pragma GCC diagnostic pop

    /* Send the buffer. */
    u8g2_SendBuffer(&ctx->u8g2);
}
