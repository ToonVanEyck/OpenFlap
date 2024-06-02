#ifndef CHAIN_COMM_ABI_H
#define CHAIN_COMM_ABI_H

#include <stddef.h>
#include <stdint.h>

#define ABI_VERSION 2

#define MAX_PROPERTIES (64) // (6 bits)
#define WRITE_HEADER_LEN 1  // Write header is 1 bytes long: [HEADER]
#define READ_HEADER_LEN 3   // Read header is 3 bytes long: [HEADER] [CNT_LSB] [CNT_MSB]
#define ACKNOWLEDGE_LEN 1
#define CHAIN_COM_MAX_LEN 256
#define SEQUENTIAL_WRITE_TRIGGER_DELAY_MS 50
#define SEQUENTIAL_WRITE_TRIGGER_DELAY_EXTRA_MS 5
#define SEQUENTIAL_WRITE_PASSTHROUGH_PERIOD_MS                                                                         \
    (SEQUENTIAL_WRITE_TRIGGER_DELAY_MS - SEQUENTIAL_WRITE_TRIGGER_DELAY_EXTRA_MS)
#define MAX_COMMAND_PERIOD_MS (SEQUENTIAL_WRITE_TRIGGER_DELAY_MS + SEQUENTIAL_WRITE_TRIGGER_DELAY_EXTRA_MS)
#define ACK 0x00

typedef enum __attribute__((__packed__)) {
    do_nothing,
    property_readAll,
    property_writeSequential,
    property_writeAll,
} moduleAction_t;

#define GENERATE_PROPERTY_ENUM(ENUM, NAME, SIZE) ENUM,
#define GENERATE_PROPERTY_NAME(ENUM, NAME, SIZE) NAME,
#define GENERATE_PROPERTY_SIZE(ENUM, NAME, SIZE) SIZE,

#define MODULE_PROPERTY(PROPERTY)                                                                                      \
    PROPERTY(no_property, NULL, 0)                                                                                     \
    PROPERTY(firmware_property, "firmware", 130)                                                                       \
    PROPERTY(command_property, "command", 1)                                                                           \
    PROPERTY(columnEnd_property, "columnEnd", 1)                                                                       \
    PROPERTY(characterMapSize_property, "characterMapSize", 1)                                                         \
    PROPERTY(characterMap_property, "characterMap", 200)                                                               \
    PROPERTY(offset_property, "offset", 1)                                                                             \
    PROPERTY(vtrim_property, "vtrim", 1)                                                                               \
    PROPERTY(character_property, "character", 1)                                                                       \
    PROPERTY(baseSpeed_property, "baseSpeed", 1)                                                                       \
    PROPERTY(end_of_properties, NULL, 0)

typedef enum __attribute__((__packed__)) { MODULE_PROPERTY(GENERATE_PROPERTY_ENUM) } moduleProperty_t;

#ifdef DO_GENERATE_PROPERTY_NAMES
static const char *propertyNames[] = {MODULE_PROPERTY(GENERATE_PROPERTY_NAME)};
#endif

static const uint8_t propertySizes[] = {MODULE_PROPERTY(GENERATE_PROPERTY_SIZE)};

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
            char data[CHAIN_COM_MAX_LEN - 1];
        } structured;
        char raw[CHAIN_COM_MAX_LEN];
    };
    uint16_t size;
} chainCommMessage_t;

typedef enum {
    no_command,
    reboot_command, // reboot the modules
} moduleCommand_t;

#endif
