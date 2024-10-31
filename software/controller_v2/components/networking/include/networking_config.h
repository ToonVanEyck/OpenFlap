/**
 * \file networking_config.h
 *
 * Configuration structure used by the networking component.
 */

#pragma once

#include <esp_err.h>
#include <stdbool.h>
#include <stdint.h>

/**
 * Network configuration structure.
 */
typedef struct {
    struct {
        struct {
            bool enable;    /**< Enable wifi station mode. */
            char *ssid;     /**< SSID of the network to connect to. */
            char *password; /**< Password of the network to connect to. */
        } station;          /**< Config for wifi station mode. */
        struct {
            bool enable;    /**< Enable wifi access point mode. */
            char *ssid;     /**< SSID of the network to host. */
            char *password; /**< Password of the network to host. */
        } access_point;     /**< Config for wifi access_point mode. */
    } wifi;                 /**< Config for wifi. */
    struct {
        bool enable; /**< Enable ethernet mode. */
    } ethernet;      /**< Config for ethernet. */
} networking_config_t;
