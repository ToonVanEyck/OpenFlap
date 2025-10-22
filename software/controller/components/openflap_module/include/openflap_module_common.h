#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "esp_log.h"
#include "openflap_properties.h"

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
 * Contains one flash page at a time.
 */
typedef struct {
    uint16_t index; /**< index of the currently contained flash page. */
    uint8_t *data;  /**< Data in the firmware property.*/
    uint16_t reff_cnt /**< Reference count of this property instance. */;
} firmware_update_property_t;

/** Encoder offset properties. */
typedef uint8_t offset_property_t;

/** Character index properties. */
typedef uint8_t character_index_property_t;

/** Color structure. */
typedef struct {
    uint8_t red;   /**< Red color. */
    uint8_t green; /**< Green color. */
    uint8_t blue;  /**< Blue color. */
} color_t;

/** Color property. */
typedef struct {
    color_t foreground; /**< Foreground color. */
    color_t background; /**< Background color. */
} color_property_t;

/** Motion property. */
typedef struct {
    uint8_t speed_min;           /**< Minimum speed of the system. */
    uint8_t speed_max;           /**< Maximum speed of the system. */
    uint8_t distance_ramp_start; /**< Use minimum speed when distance is equal or below the value. */
    uint8_t distance_ramp_stop;  /**< Use maximum speed when distance is equal or above the value. */
} motion_property_t;

/** IR threshold property. */
typedef struct {
    int16_t lower; /**< Lower limit of the IR threshold. */
    int16_t upper; /**< Upper limit of the IR threshold. */
} ir_threshold_property_t;

/** Minimum rotation property. */
typedef uint8_t minimum_rotation_property_t;

/**
 * \brief Module structure.
 */
typedef struct {
    firmware_version_property_t *firmware_version; /**< Firmware version property. */
    uint32_t firmware_crc;                         /**< CRC of the firmware. */
    firmware_update_property_t *firmware_update;   /**< Firmware update property. */
    command_property_cmd_t command;                /**< Command property. */
    bool column_end;                               /**< Column end property. */
    character_set_property_t *character_set;       /**< Character set property. */
    character_index_property_t character_index;    /**< Current index of the character in the charactermap. */
    offset_property_t offset;                      /**< offset property. */
    color_property_t color;                        /**< Color property. */
    motion_property_t motion;                      /**< Motion property. */
    minimum_rotation_property_t minimum_rotation;  /**< Minimum rotation property. */
    ir_threshold_property_t ir_threshold;          /**< IR threshold property. */
    /** Indicates witch properties need to be synchronized by writing to actual modules. */
    uint64_t sync_prop_write_required;
} module_t;

//---------------------------------------------------------------------------------------------------------------------
