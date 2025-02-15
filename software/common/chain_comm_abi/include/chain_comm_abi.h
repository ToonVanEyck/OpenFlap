#ifndef CHAIN_COMM_ABI_H
#define CHAIN_COMM_ABI_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define ABI_VERSION 2

#define CHAIN_COM_MAX_LEN      (256)  /**< Maximum length of a chain communication message. */
#define CHAIN_COMM_TIMEOUT_ACK (0xFF) /**< Value used as acknowledge. */
#define CHAIN_COMM_TIMEOUT_MS  (250)  /**< Time after which a timeout event occurs. */

typedef enum __attribute__((__packed__)) {
    do_nothing,
    property_readAll,
    property_writeSequential,
    property_writeAll,
} chain_comm_action_t;

typedef struct {
    bool dynamic_property_size;
    uint16_t static_property_size;
} chain_comm_binary_attributes_t;

#define GENERATE_PROPERTY_ENUM(ENUM, NAME, READ_ATTR, WRITE_ATTR)       ENUM,
#define GENERATE_PROPERTY_NAME(ENUM, NAME, READ_ATTR, WRITE_ATTR)       NAME,
#define GENERATE_PROPERTY_READ_ATTR(ENUM, NAME, READ_ATTR, WRITE_ATTR)  {READ_ATTR},
#define GENERATE_PROPERTY_WRITE_ATTR(ENUM, NAME, READ_ATTR, WRITE_ATTR) {WRITE_ATTR},

#define PROP_ATTR_STATIC(_s) .dynamic_property_size = false, .static_property_size = (_s)
#define PROP_ATTR_DYNAMIC()  .dynamic_property_size = true, .static_property_size = 0
#define PROP_ATTR_NONE()     0, 0

/**
 * ENUM , NAME, READ ATTRIBUTES, WRITE ATTRIBUTES
 */
#define MODULE_PROPERTY(PROPERTY)                                                                                      \
    PROPERTY(PROPERTY_NONE, NULL, PROP_ATTR_NONE(), PROP_ATTR_NONE())                                                  \
    PROPERTY(PROPERTY_FIRMWARE_VERSION, "firmware_version", PROP_ATTR_DYNAMIC(), PROP_ATTR_NONE())                     \
    PROPERTY(PROPERTY_FIRMWARE_UPDATE, "firmware_update", PROP_ATTR_NONE(), PROP_ATTR_STATIC(130))                     \
    PROPERTY(PROPERTY_COMMAND, "command", PROP_ATTR_NONE(), PROP_ATTR_STATIC(1))                                       \
    PROPERTY(PROPERTY_MODULE_INFO, "module_info", PROP_ATTR_STATIC(1), PROP_ATTR_NONE())                               \
    PROPERTY(PROPERTY_CHARACTER_SET, "character_set", PROP_ATTR_DYNAMIC(), PROP_ATTR_DYNAMIC())                        \
    PROPERTY(PROPERTY_CHARACTER, "character", PROP_ATTR_STATIC(1), PROP_ATTR_STATIC(1))                                \
    PROPERTY(PROPERTY_OFFSET, "offset", PROP_ATTR_STATIC(1), PROP_ATTR_STATIC(1))                                      \
    PROPERTY(PROPERTY_COLOR, "color", PROP_ATTR_STATIC(2), PROP_ATTR_STATIC(2))                                        \
    PROPERTY(PROPERTY_MOTION, "motion", PROP_ATTR_STATIC(4), PROP_ATTR_STATIC(4))                                      \
    PROPERTY(PROPERTY_MINIMUM_ROTATION, "minimum_rotation", PROP_ATTR_STATIC(1), PROP_ATTR_STATIC(1))                  \
    PROPERTY(PROPERTIES_MAX, NULL, PROP_ATTR_NONE(), PROP_ATTR_NONE())

typedef enum __attribute__((__packed__)) { MODULE_PROPERTY(GENERATE_PROPERTY_ENUM) } property_id_t;

uint8_t get_property_size(property_id_t property);
const char *chain_comm_property_name_by_id(property_id_t property);
property_id_t chain_comm_property_id_by_name(const char *name);

const chain_comm_binary_attributes_t *chain_comm_property_read_attributes_get(property_id_t property);
const chain_comm_binary_attributes_t *chain_comm_property_write_attributes_get(property_id_t property);

typedef union __attribute__((__packed__)) {
    uint8_t raw;
    struct {
        property_id_t property : 6;
        chain_comm_action_t action : 2;
    };
} chain_comm_msg_header_t;

#define NAMED_ENUM_ENUM(ENUM, NAME) ENUM,
#define NAMED_ENUM_NAME(ENUM, NAME) NAME,

#define COMMAND_PROPERTY(NAMED_ENUM)                                                                                   \
    NAMED_ENUM(CMD_NO_COMMAND, "no_command")                                                                           \
    NAMED_ENUM(CMD_REBOOT, "reboot")                                                                                   \
    NAMED_ENUM(CMD_MAX, NULL)

typedef enum { COMMAND_PROPERTY(NAMED_ENUM_ENUM) } command_property_t;
const char *chain_comm_command_name_get(command_property_t command);

#define MODULE_TYPE_PROPERTY(NAMED_ENUM)                                                                               \
    NAMED_ENUM(MODULE_TYPE_UNDEFINED, "undefined")                                                                     \
    NAMED_ENUM(MODULE_TYPE_SPLITFLAP, "splitflap")                                                                     \
    NAMED_ENUM(MODULE_TYPE_MAX, NULL)

typedef enum { MODULE_TYPE_PROPERTY(NAMED_ENUM_ENUM) } module_type_t;
const char *chain_comm_module_type_name_get(module_type_t module_type);

/**
 * \brief Module info.
 */
typedef struct __attribute__((__packed__)) {
    union {
        struct {
            bool column_end : 1;    /**< Indicates that this module is the last one in a column. */
            module_type_t type : 4; /**< The type of the module. */
            uint8_t reserved : 3;   /**< Reserved for future use. */
        };
        uint8_t raw; /**< The raw value of the module info. */
    };
} module_info_property_t;

#endif
