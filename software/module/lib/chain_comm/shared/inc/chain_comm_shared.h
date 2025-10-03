#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define ABI_VERSION 2

#define CC_PROPERTY_SIZE_MAX  (256) /**< Maximum length of a property. */
#define CHAIN_COMM_TIMEOUT_MS (50)  /**< Time after which a timeout event occurs. */

#define CC_PROP_SIZE_DYNAMIC    {.is_dynamic = true, .static_size = 0}
#define CC_PROP_SIZE_STATIC(_s) {.is_dynamic = false, .static_size = (_s)}
#define CC_PROP_SIZE_NONE       {.is_dynamic = false, .static_size = 0}

typedef void (*uart_read_timeout_set_cb_t)(void *uart_userdata, uint32_t timeout_ms);
typedef size_t (*uart_read_cb_t)(void *uart_userdata, uint8_t *data, size_t size);
typedef size_t (*uart_cnt_readable_cb_t)(void *uart_userdata);
typedef size_t (*uart_write_cb_t)(void *uart_userdata, const uint8_t *data, size_t size);
typedef size_t (*uart_cnt_writable_cb_t)(void *uart_userdata);
typedef bool (*uart_tx_buff_empty_cb_t)(void *uart_userdata);
typedef bool (*uart_is_busy_cb_t)(void *uart_userdata);

/**
 * \brief Chain communication property callback.
 *
 * \param[in] buf Pointer to the data buffer.
 * \param[inout] size Pointer to the size of the data buffer. This must be set in the handler if the property has a
 *      dynamic length.
 */
typedef void (*cc_prop_handler_cb_t)(uint16_t node_idx, uint8_t *buf, uint16_t *size, void *userdata);

typedef enum __attribute__((__packed__)) {
    property_readAll         = 0,
    property_writeSequential = 1,
    property_writeAll        = 2,
    returnCode               = 3,
} cc_action_t;

typedef struct {
    bool is_dynamic;
    uint16_t static_size;
} cc_prop_size_t;

typedef uint16_t cc_prop_id_t;

typedef struct {
    cc_prop_handler_cb_t get;
    cc_prop_handler_cb_t set;
} cc_prop_handler_t;

typedef struct {
    cc_prop_size_t read_size;
    cc_prop_size_t write_size;
    char *name;
} cc_prop_attr_t;

typedef struct {
    cc_prop_handler_t handler;
    cc_prop_attr_t attribute;
} cc_prop_t;

typedef enum {
    CC_RC_SUCCESS        = 0,
    CC_RC_ERROR_CHECKSUM = (uint8_t)(1 << 0),
} cc_ret_code_t;

typedef union {
    uint8_t raw;
    struct __attribute__((__packed__)) {
        cc_prop_id_t property : 6;
        // bool extended_id : 1;
        cc_action_t action : 2;
    };
} cc_msg_header_t;

typedef union {
    uint8_t raw;
    struct __attribute__((__packed__)) {
        cc_ret_code_t rc : 6;
        cc_action_t action : 2;
    };
} cc_msg_return_code_t;
