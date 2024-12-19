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
} moduleAction_t;

typedef struct {
    bool multipart;
    bool dynamic_size;
    uint8_t static_size;
} chain_comm_binary_attributes_t;

#define GENERATE_PROPERTY_ENUM(ENUM, NAME, READ_ATTR, WRITE_ATTR)       ENUM,
#define GENERATE_PROPERTY_NAME(ENUM, NAME, READ_ATTR, WRITE_ATTR)       NAME,
#define GENERATE_PROPERTY_READ_ATTR(ENUM, NAME, READ_ATTR, WRITE_ATTR)  (chain_comm_binary_attributes_t) {READ_ATTR},
#define GENERATE_PROPERTY_WRITE_ATTR(ENUM, NAME, READ_ATTR, WRITE_ATTR) (chain_comm_binary_attributes_t) {WRITE_ATTR},

#define PROP_ATTR_NONE            0, 0, 0
#define PROP_ATTR_MULTIPART       1, 0, 0
#define PROP_ATTR_DYNAMIC_SIZE    0, 1, 0
#define PROP_ATTR_STATIC_SIZE(_s) 0, 0, (_s)

/**
 * ENUM , NAME, READ ATTRIBUTES, WRITE ATTRIBUTES
 */
#define MODULE_PROPERTY(PROPERTY)                                                                                      \
    PROPERTY(PROPERTY_NONE, NULL, PROP_ATTR_NONE, PROP_ATTR_NONE)                                                      \
    PROPERTY(PROPERTY_FIRMWARE, "firmware", PROP_ATTR_STATIC_SIZE(0), PROP_ATTR_STATIC_SIZE(130))                      \
    PROPERTY(PROPERTY_COMMAND, "command", PROP_ATTR_STATIC_SIZE(0), PROP_ATTR_STATIC_SIZE(1))                          \
    PROPERTY(PROPERTY_MODULE_INFO, "module_info", PROP_ATTR_STATIC_SIZE(1), PROP_ATTR_STATIC_SIZE(0))                  \
    PROPERTY(PROPERTY_CHARACTER_SET, "character_set", PROP_ATTR_DYNAMIC_SIZE, PROP_ATTR_DYNAMIC_SIZE)                  \
    PROPERTY(PROPERTY_CHARACTER, "character", PROP_ATTR_STATIC_SIZE(1), PROP_ATTR_STATIC_SIZE(1))                      \
    PROPERTY(PROPERTY_CALIBRATION, "calibration", PROP_ATTR_STATIC_SIZE(2), PROP_ATTR_STATIC_SIZE(2))                  \
    PROPERTY(PROPERTIES_MAX, NULL, PROP_ATTR_NONE, PROP_ATTR_NONE)

typedef enum __attribute__((__packed__)) { MODULE_PROPERTY(GENERATE_PROPERTY_ENUM) } property_id_t;

uint8_t get_property_size(property_id_t property);
const char *chain_comm_property_name_get(property_id_t property);
const chain_comm_binary_attributes_t *chain_comm_property_read_attributes_get(property_id_t property);
const chain_comm_binary_attributes_t *chain_comm_property_write_attributes_get(property_id_t property);

typedef union __attribute__((__packed__)) {
    uint8_t raw;
    struct {
        uint8_t property : 6;
        uint8_t action : 2;
    } field;
} chainCommHeader_t;

typedef struct {
    union {
        struct {
            chainCommHeader_t header;
            uint8_t data[CHAIN_COM_MAX_LEN - 1];
        } structured;
        uint8_t raw[CHAIN_COM_MAX_LEN];
    };
    uint16_t size;
} chainCommMessage_t;

/** Enum with supported commands. */
typedef enum {
    CMD_NO_COMMAND, /**< No command. */
    CMD_REBOOT,     /**< Reboot the modules. */
} command_property_t;

#endif
