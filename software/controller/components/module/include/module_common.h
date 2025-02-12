#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "chain_comm_abi.h"
#include "esp_log.h"

/** Character Set properties */
typedef struct {
    uint8_t size; /**< Size of the character set ( 1/4 of character_set array. ) */
    /** List of supported characters, 4 bytes will be allocated per character so all UTF-8 characters are supported.  */
    uint8_t *data;
    uint16_t reff_cnt /**< Reference count of this property instance. */;
} character_set_property_t;

/**
 * \brief Firmware version property.
 */
typedef struct {
    char *str; /**< Version of the firmware. */
    uint16_t reff_cnt /**< Reference count of this property instance. */;
} firmware_version_property_t;

/**
 * \brief Firmware update property.
 *
 * Contains one flash page at a time..
 */
typedef struct {
    uint16_t index; /**< index of the currently contained flash page. */
    uint8_t *data;  /**< Data in the firmware property.*/
    uint16_t reff_cnt /**< Reference count of this property instance. */;
} firmware_update_property_t;

/** Calibration properties */
typedef struct {
    uint8_t offset; /**< Offset between the actual character and the index on the encoder wheel. */
} calibration_property_t;

/**
 * \brief Module structure.
 */
typedef struct {
    module_info_property_t module_info;            /**< Module info property. */
    command_property_t command;                    /**< Command property. */
    firmware_update_property_t *firmware_update;   /**< Firmware update property. */
    firmware_version_property_t *firmware_version; /**< Firmware version property. */
    character_set_property_t *character_set;       /**< Character set property. */
    calibration_property_t calibration;            /**< Calibration property. */
    /**< The index of the character in the charactermap that is currently being displayed. */
    uint8_t character_index;
    /** Indicates witch properties need to be synchronized by writing to actual modules. */
    uint64_t sync_properties_write_seq_required;
} module_t;

//---------------------------------------------------------------------------------------------------------------------
