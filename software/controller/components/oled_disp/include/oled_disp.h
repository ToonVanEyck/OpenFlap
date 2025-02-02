#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "u8g2.h"
#include "u8g2_esp32_hal.h"

/**
 * \brief The type of message to be displayed on the OLED display.
 */
typedef enum {
    OLED_DISP_MSG_HOME_SCREEN = 0,  /**< The home screen. */
    OLED_DISP_MEG_WIFI_MODE_SCREEN, /**< The wifi mode screen. */
    OLED_DISP_MSG_AP_SCREEN,        /**< The wifi access point mode screen. */
    OLED_DISP_MSG_STA_SCREEN,       /**< The wifi station mode screen. */
    OLED_DISP_MSG_LOADING_SCREEN,   /**< A loading screen. */
} oled_disp_msg_type_t;

typedef struct {
    char title[16];        /**< The title shown on the screen. */
    uint8_t version_major; /**< The major version number. */
    uint8_t version_minor; /**< The minor version number. */
    uint8_t version_patch; /**< The patch version number. */
} oled_disp_msg_home_t;

typedef struct {
    char title[32];   /**< The title shown on the screen. */
    uint8_t progress; /**< The progress of the loading screen. */
} oled_disp_msg_loading_t;

typedef struct {
    bool wifi_ap_mode;  /**< The Wifi is in AP mode */
    bool wifi_sta_mode; /**< The Wifi is in STA mode */
} oled_disp_msg_wifi_mode_t;

typedef struct {
    char ssid[16];      /**< The ssid being hosted by the esp. */
    uint8_t client_cnt; /**< The number of clients connected. */
} oled_disp_msg_wifi_ap_t;

typedef struct {
    char ssid[16];  /**< The ssid of the network being connected to. */
    bool connected; /**< The esp is connected to the network. */
    char ip[16];    /**< The ip of the esp. */
} oled_disp_msg_wifi_sta_t;

typedef struct {
    oled_disp_msg_type_t type;
    union {
        oled_disp_msg_home_t home;
        oled_disp_msg_loading_t loading;
        oled_disp_msg_wifi_mode_t wifi_mode;
        oled_disp_msg_wifi_ap_t wifi_ap;
        oled_disp_msg_wifi_sta_t wifi_sta;
    };
} oled_disp_msg_t;

/**
 * \brief The OLED display context.
 */
typedef struct {
    u8g2_esp32_hal_t u8g2_esp32_hal; /**< u8g2 display driver HAL handle. */
    u8g2_t u8g2;                     /**< u8g2 display driver handle. */
    QueueHandle_t queue;             /**< Queue for the OLED display task. */
    TaskHandle_t task;               /**< Task handle for the OLED display task. */
} oled_disp_ctx_t;

// typedef uintptr_t oled_disp_handle_t;

/**
 * \brief Initialize the OLED display.
 *
 * \param[out] ctx the context for the OLED display to be used in subsequent calls.
 *
 * \return ESP_OK on success, ESP_FAIL otherwise.
 */
esp_err_t oled_disp_init(oled_disp_ctx_t *ctx);

/**
 * \brief Cleanup the OLED display.
 *
 * \param[out] ctx the context for the OLED display to cleanup.
 *
 * \return ESP_OK on success, ESP_FAIL otherwise.
 */
esp_err_t oled_disp_clean(oled_disp_ctx_t *ctx);

/**
 * \brief Display the home screen on the OLED display.
 *
 * \param[in] ctx the context for the OLED display.
 * \param[in] title the title to display on the home screen.
 * \param[in] version_major the major version number to display on the home screen.
 * \param[in] version_minor the minor version number to display on the home screen.
 * \param[in] version_patch the patch version number to display on the home screen.
 *
 * \return ESP_OK on success, ESP_FAIL otherwise.
 */
esp_err_t oled_disp_home(oled_disp_ctx_t *ctx, const char *title, uint8_t version_major, uint8_t version_minor,
                         uint8_t version_patch);

/**
 * \brief Display the loading screen on the OLED display.
 *
 * \param[in] ctx the context for the OLED display.
 * \param[in] title the title to display on the loading screen.
 * \param[in] progress the progress to display on the loading screen.
 *
 * \return ESP_OK on success, ESP_FAIL otherwise.
 */
esp_err_t oled_disp_loading(oled_disp_ctx_t *ctx, const char *title, uint8_t progress);

/**
 * \brief Display the wifi mode screen on the OLED display.
 *
 * \param[in] ctx the context for the OLED display.
 * \param[in] wifi_ap_mode the wifi ap mode to display on the wifi mode screen.
 * \param[in] wifi_sta_mode the wifi sta mode to display on the wifi mode screen.
 *
 * \return ESP_OK on success, ESP_FAIL otherwise.
 */

esp_err_t oled_disp_wifi_mode(oled_disp_ctx_t *ctx, bool wifi_ap_mode, bool wifi_sta_mode);

/**
 * \brief Display the wifi ap screen on the OLED display.
 *
 * \param[in] ctx the context for the OLED display.
 * \param[in] ssid the ssid to display on the wifi ap screen.
 * \param[in] client_cnt the client count to display on the wifi ap screen.
 *
 * \return ESP_OK on success, ESP_FAIL otherwise.
 */
esp_err_t oled_disp_wifi_ap(oled_disp_ctx_t *ctx, const char *ssid, uint8_t client_cnt);

/**
 * \brief Display the wifi sta screen on the OLED display.
 *
 * \param[in] ctx the context for the OLED display.
 * \param[in] ssid the ssid to display on the wifi sta screen.
 * \param[in] connected the connected status to display on the wifi sta screen.
 * \param[in] ip the ip to display on the wifi sta screen.
 *
 * \return ESP_OK on success, ESP_FAIL otherwise.
 */
esp_err_t oled_disp_wifi_sta(oled_disp_ctx_t *ctx, const char *ssid, bool connected, const char *ip);
