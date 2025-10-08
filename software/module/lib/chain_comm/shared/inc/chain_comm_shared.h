#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

#define ABI_VERSION 2

#define CC_PROPERTY_SIZE_MAX (254) /**< Maximum length of a property. */

/** Additional overhead available for COBS encoding and checksum. */
#define CC_COBS_OVERHEAD_SIZE ((CC_PROPERTY_SIZE_MAX / 0xff) + 2)
#define CC_PAYLOAD_SIZE_MAX   (CC_PROPERTY_SIZE_MAX + CC_COBS_OVERHEAD_SIZE) /**< Maximum size of a payload. */

#ifndef CHAIN_COMM_TIMEOUT_MS
#define CHAIN_COMM_TIMEOUT_MS (500) /**< Time after which a timeout event occurs. */
#endif

#define CC_PROP_SIZE_DYNAMIC    {.is_dynamic = true, .static_size = 0}
#define CC_PROP_SIZE_STATIC(_s) {.is_dynamic = false, .static_size = (_s)}
#define CC_PROP_SIZE_NONE       {.is_dynamic = false, .static_size = 0}

#define CC_ACTION_HEADER_SIZE (3) /**< Size of the action header. */
#define CC_SYNC_HEADER_SIZE   (1) /**< Size of the sync header. */

#define CC_CHECKSUM_OK (0x00) /**< Checksum value indicating no error. */

/**
 * \brief Chain communication UART set read timeout callback.
 * Set the timeout for the blocking master read function.
 *
 * \param[in] uart_userdata Pointer to uart ctx.
 * \param[in] timeout_ms Timeout in milliseconds.
 */
typedef void (*uart_read_timeout_set_cb_t)(void *uart_userdata, uint32_t timeout_ms);

/**
 * \brief Chain communication UART read callback.
 *
 * \note The master requires a blocking read with timeout functionality. The node implementation must be non-blocking.
 *
 * \param[in] uart_userdata Pointer to uart ctx.
 * \param[in] data Pointer to the data buffer.
 * \param[in] size Size of the data buffer.
 *
 * \return Number of bytes read, or -1 on error.
 */
typedef ssize_t (*uart_read_cb_t)(void *uart_userdata, uint8_t *data, size_t size);

/**
 * \brief Chain communication UART rx buffer space used callback.
 *
 * \param[in] uart_userdata Pointer to uart ctx.
 *
 * \return Number of bytes available in the rx buffer.
 */
typedef ssize_t (*uart_cnt_readable_cb_t)(void *uart_userdata);

/**
 * \brief Chain communication UART write callback.
 *
 * \param[in] uart_userdata Pointer to uart ctx.
 * \param[in] data Pointer to the data buffer.
 * \param[in] size Size of the data buffer.
 *
 * \return Number of bytes written, or -1 on error.
 */
typedef ssize_t (*uart_write_cb_t)(void *uart_userdata, const uint8_t *data, size_t size);

/**
 * \brief Chain communication UART tx buffer space available callback.
 *
 * \param[in] uart_userdata Pointer to uart ctx.
 *
 * \return Number of bytes available in the tx buffer.
 */
typedef ssize_t (*uart_cnt_writable_cb_t)(void *uart_userdata);

/**
 * \brief Chain communication UART tx pending callback.
 *
 * \param[in] uart_userdata Pointer to uart ctx.
 *
 * \return false if tx pending, true if tx buffer empty.
 */
typedef bool (*uart_tx_buff_empty_cb_t)(void *uart_userdata);

/**
 * \brief Chain communication UART busy callback.
 *
 * \param[in] uart_userdata Pointer to uart ctx.
 */
typedef bool (*uart_is_busy_cb_t)(void *uart_userdata);

/**
 * \brief Chain communication property GET / SET handler callback.
 *
 * \param[in] node_idx Index of the node for which the property is requested. Only used by the master.
 * \param[in] buf Pointer to the data buffer.
 * \param[inout] size Pointer to the size of the data buffer.
 * \param[in] userdata Pointer to user data.
 */
typedef void (*cc_prop_handler_cb_t)(uint16_t node_idx, uint8_t *buf, size_t *size, void *userdata);

typedef enum {
    CC_ACTION_READ      = 0, /**< Read property data from all nodes. */
    CC_ACTION_WRITE     = 1, /**< Write (different) property data to all nodes. */
    CC_ACTION_BROADCAST = 2, /**< Broadcast / Write (the same) property data to all nodes. */
    CC_ACTION_SYNC      = 3, /**< Synchronization action used for error checking and latching data. */
} cc_action_t;

typedef enum {
    CC_SYNC_SYNC       = 0, /**< A synchronisation byte, will be passed after the nodes exit from the EXEC state. */
    CC_SYNC_EXEC       = 1, /**< Execute previously received data. */
    CC_SYNC_RESERVED_1 = 2, /**< Reserved for future use. */
    CC_SYNC_RESERVED_2 = 3, /**< Reserved for future use. */
} cc_sync_type_t;

typedef enum {
    CC_SYNC_ERR_NONE          = 0,
    CC_SYNC_ERR_TIMEOUT       = (1 << 0), /**< A timeout occurred. */
    CC_SYNC_ERR_TRANSMISSION  = (1 << 1), /**< A transmission / checksum error occurred. */
    CC_SYNC_ERR_INVALID_DATA  = (1 << 2), /**< The property handler could not handle the data. */
    CC_SYNC_ERR_NOT_SUPPORTED = (1 << 3), /**< The Property is not supported for the active action. */
} cc_sync_error_code_t;

/**
 * \brief Chain communication property id type.
 */
typedef uint16_t cc_prop_id_t;

/**
 * \brief Chain communication property handler.
 */
typedef struct {
    cc_prop_handler_cb_t get; /**< Read the property from the node or buffer into a buffer. */
    cc_prop_handler_cb_t set; /**< Write the property to the node or buffer from a buffer. */
} cc_prop_handler_t;

/**
 * \brief Chain communication property attributes.
 */
typedef struct {
    char *name; /**< Name of the property. */
} cc_prop_attr_t;

/**
 * \brief Chain communication property structure.
 */
typedef struct {
    cc_prop_handler_t handler; /**< Property handler functions. */
    cc_prop_attr_t attribute;  /**< Property attributes. */
} cc_prop_t;

/**
 * \brief Bit field definitions for the message header.
 *
 * \code
 * 23                                    16 15            8 7                    0
 * +-------------+-------------------------+---------------+---------------------+
 * |            BYTE 0                     |    BYTE 1     |        BYTE 2       |
 * +-------------+-------------------------+-----+---------+-------+-------------+
 * | ACTION (3b) | RESERVED (1b) | PROPERTY (6b) |  NODE_CNT (13b) | PARITY (1b) |
 * +-------------+---------------+---------------+-----------------+-------------+
 * 23          22 21           21 20           14 13              1 0            0
 * \endcode
 * \note
 * The node count is split into 7 MSB bits and 6 LSB bits. This is done because we must always transmit the node count
 * LSB before the MSB.
 */
typedef struct {
    uint8_t raw[CC_ACTION_HEADER_SIZE]; /* Raw access to the header bytes. */
} cc_msg_header_t;

/**
 * \brief Encode a payload using COBS encoding.
 *
 * \param[out] dst Pointer to the destination buffer.
 * \param[inout] dst_len Pointer to the length of the destination buffer. Will be updated with the actual length used.
 * \param[in] src Pointer to the source buffer.
 * \param[in] src_len Length of the source buffer.
 *
 * \return true if encoding was successful, false otherwise.
 */
bool cc_payload_cobs_encode(uint8_t *dst, size_t *dst_len, const uint8_t *src, size_t src_len);

/**
 * \brief Decode a payload using COBS encoding.
 *
 * \param[out] dst Pointer to the destination buffer.
 * \param[inout] dst_len Pointer to the length of the destination buffer. Will be updated with the actual length used.
 * \param[in] src Pointer to the source buffer.
 * \param[in] src_len Length of the source buffer.
 *
 * \return true if decoding was successful, false otherwise.
 */
bool cc_payload_cobs_decode(uint8_t *dst, size_t *dst_len, const uint8_t *src, size_t src_len);

/**
 * \brief Update a checksum with a new byte of data.
 *
 * \param[inout] checksum Pointer to the checksum variable.
 * \param[in] data New byte of data to update the checksum with.
 */
void cc_checksum_update(uint8_t *checksum, uint8_t data);

/**
 * \brief Calculate the checksum of a data buffer.
 * Calculates an 8 bit checksum of an array of data. If the array is terminated with the checksum, the result will be
 * 0xFF.
 *
 * \param[in] data Pointer to the data buffer.
 * \param[in] size Size of the data buffer.
 *
 * \return The calculated checksum.
 */
uint8_t cc_checksum_calculate(const uint8_t *data, size_t size);

/**
 * \brief Extract the action type from a message header.
 *
 * \param[in] header The message header to extract the action type from.
 *
 * \return The action type.
 */
cc_action_t cc_header_action_get(cc_msg_header_t header);

/**
 * \brief Set the action type in a message header.
 *
 * \param[inout] header Pointer to the message header to set the action type for.
 * \param[in] action The action type to set.
 */
void cc_header_action_set(cc_msg_header_t *header, cc_action_t action);

/**
 * \brief Extract the property id from a message header.
 *
 * \param[in] header The message header to extract the property id from.
 *
 * \return The property id.
 */
cc_prop_id_t cc_header_property_get(cc_msg_header_t header);

/**
 * \brief Set the property id in a message header.
 *
 * \param[inout] header Pointer to the message header to set the property id for.
 * \param[in] property The property id to set.
 */
void cc_header_property_set(cc_msg_header_t *header, cc_prop_id_t property);

/**
 * \brief Extract the node count from a message header.
 *
 * \param[in] header The message header to extract the node count from.
 *
 * \return The node count.
 */
uint16_t cc_header_node_cnt_get(cc_msg_header_t header);

/**
 * \brief Set the node count in a message header.
 *
 * \param[inout] header Pointer to the message header to set the node count for.
 * \param[in] node_cnt The node count to set.
 */
void cc_header_node_cnt_set(cc_msg_header_t *header, uint16_t node_cnt);

/**
 * \brief Verify the parity of a message header.
 *
 * \param[in] header The message header to verify.
 *
 * \return true if the parity is valid, false otherwise.
 */
bool cc_header_parity_check(cc_msg_header_t header);

/**
 * \brief Set the parity bit in a message header.
 *
 * \param[inout] header Pointer to the message header to set the parity bit for.
 */
void cc_header_parity_set(cc_msg_header_t *header);