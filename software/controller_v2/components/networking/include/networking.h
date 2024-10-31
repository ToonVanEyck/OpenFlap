/**
 * \file networking.h
 *
 * The networking component adds support for connecting the system to a network over wifi or ethernet.
 * WiFi can be used to join a network as a client or host a network as an access point.
 * Ethernet can be used when connecting in a qemu environment.
 */

#pragma once

#include "networking_config.h"
#include "networking_default_config.h"

/**
 * \brief Setup networking.
 *
 * \param[in] config  Configuration for the network.
 */
esp_err_t networking_setup(const networking_config_t *config);

/**
 * \brief Wait for a connection to be established.
 *
 * \param[in] timeout_ms Timeout in milliseconds.
 *
 * \retval ESP_OK if a connection was established
 * \retval ESP_ERR_TIMEOUT if the timeout was reached.
 * \retval ESP_FAIL if the connection failed.
 */
esp_err_t networking_wait_for_connection(uint32_t timeout_ms);