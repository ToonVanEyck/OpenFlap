#ifndef CHAIN_COMM_ABI_H
#define CHAIN_COMM_ABI_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define ABI_VERSION 2

#define MAX_PROPERTIES                          (64) // (6 bits)
#define WRITE_HEADER_LEN                        1    // Write header is 1 bytes long: [HEADER]
#define READ_HEADER_LEN                         3    // Read header is 3 bytes long: [HEADER] [CNT_LSB] [CNT_MSB]
#define ACKNOWLEDGE_LEN                         1
#define CHAIN_COM_MAX_LEN                       256
#define SEQUENTIAL_WRITE_TRIGGER_DELAY_MS       50
#define SEQUENTIAL_WRITE_TRIGGER_DELAY_EXTRA_MS 5
#define SEQUENTIAL_WRITE_PASSTHROUGH_PERIOD_MS                                                                         \
    (SEQUENTIAL_WRITE_TRIGGER_DELAY_MS - SEQUENTIAL_WRITE_TRIGGER_DELAY_EXTRA_MS)
#define MAX_COMMAND_PERIOD_MS (SEQUENTIAL_WRITE_TRIGGER_DELAY_MS + SEQUENTIAL_WRITE_TRIGGER_DELAY_EXTRA_MS)
#define ACK                   0x00

typedef enum __attribute__((__packed__)) {
    do_nothing,
    property_readAll,
    property_writeSequential,
    property_writeAll,
} chain_comm_action_t;

typedef struct {
    bool dynamic_page_cnt;
    uint8_t static_page_cnt;
    bool dynamic_page_size;
    uint8_t static_page_size;
} chain_comm_binary_attributes_t;

#define GENERATE_PROPERTY_ENUM(ENUM, NAME, READ_ATTR, WRITE_ATTR)       ENUM,
#define GENERATE_PROPERTY_NAME(ENUM, NAME, READ_ATTR, WRITE_ATTR)       NAME,
#define GENERATE_PROPERTY_READ_ATTR(ENUM, NAME, READ_ATTR, WRITE_ATTR)  {READ_ATTR},
#define GENERATE_PROPERTY_WRITE_ATTR(ENUM, NAME, READ_ATTR, WRITE_ATTR) {WRITE_ATTR},

#define PAGE_CNT_STATIC(_s)  .dynamic_page_cnt = false, .static_page_cnt = (_s)
#define PAGE_CNT_DYNAMIC()   .dynamic_page_cnt = true, .static_page_cnt = 0
#define PAGE_SIZE_STATIC(_s) .dynamic_page_size = false, .static_page_size = (_s)
#define PAGE_SIZE_DYNAMIC()  .dynamic_page_size = true, .static_page_size = 0

#define PROP_ATTR(_c, _s) _c, _s
#define PROP_ATTR_SP(_s)  PROP_ATTR(PAGE_CNT_STATIC(1), PAGE_SIZE_STATIC(_s)) /* Single Page Static Size */
#define PROP_ATTR_DD      PROP_ATTR(PAGE_CNT_DYNAMIC(), PAGE_SIZE_DYNAMIC())  /* Dynamic Page count and size */
#define PROP_ATTR_NONE    0, 0, 0, 0

/**
 * ENUM , NAME, READ ATTRIBUTES, WRITE ATTRIBUTES
 */
#define MODULE_PROPERTY(PROPERTY)                                                                                      \
    PROPERTY(PROPERTY_NONE, NULL, PROP_ATTR_NONE, PROP_ATTR_NONE)                                                      \
    PROPERTY(PROPERTY_FIRMWARE, "firmware", PROP_ATTR_NONE, PROP_ATTR_SP(130))                                         \
    PROPERTY(PROPERTY_COMMAND, "command", PROP_ATTR_NONE, PROP_ATTR_SP(1))                                             \
    PROPERTY(PROPERTY_MODULE_INFO, "module_info", PROP_ATTR_SP(1), PROP_ATTR_NONE)                                     \
    PROPERTY(PROPERTY_CHARACTER_SET, "character_set", PROP_ATTR_DD, PROP_ATTR_DD)                                      \
    PROPERTY(PROPERTY_CHARACTER, "character", PROP_ATTR_SP(1), PROP_ATTR_SP(1))                                        \
    PROPERTY(PROPERTY_CALIBRATION, "calibration", PROP_ATTR_SP(1), PROP_ATTR_SP(1))                                    \
    PROPERTY(PROPERTIES_MAX, NULL, PROP_ATTR_NONE, PROP_ATTR_NONE)

typedef enum __attribute__((__packed__)) { MODULE_PROPERTY(GENERATE_PROPERTY_ENUM) } property_id_t;

uint8_t get_property_size(property_id_t property);
const char *chain_comm_property_name_get(property_id_t property);
const chain_comm_binary_attributes_t *chain_comm_property_read_attributes_get(property_id_t property);
const chain_comm_binary_attributes_t *chain_comm_property_write_attributes_get(property_id_t property);

typedef union __attribute__((__packed__)) {
    uint8_t raw;
    struct {
        property_id_t property : 6;
        chain_comm_action_t action : 2;
    } field;
} chain_comm_msg_header_t;

typedef struct __attribute__((__packed__)) {
    union {
        struct {
            chain_comm_msg_header_t header;
            uint8_t data[CHAIN_COM_MAX_LEN - 1];
        } structured;
        uint8_t raw[CHAIN_COM_MAX_LEN];
    };
    uint16_t size;
} chain_comm_msg_t;

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
        } field;
        uint8_t raw; /**< The raw value of the module info. */
    };
} module_info_property_t;

#endif