#include "chain_comm_shared.h"
#include <assert.h>
#include <string.h>

//======================================================================================================================
//                                                   PUBLIC FUNCTIONS
//======================================================================================================================

bool cc_payload_cobs_encode(uint8_t *dst, size_t *dst_len, const uint8_t *src, size_t src_len)
{
    if (!dst || !dst_len || !src) {
        return false;
    }

    size_t read_index  = 0;
    size_t write_index = 1; // First code byte will be written at 0
    size_t code_index  = 0;
    uint8_t code       = 1;

    while (read_index < src_len) {
        if (src[read_index] == 0) {
            dst[code_index] = code;
            code_index      = write_index++;
            code            = 1;
            read_index++;
            if (write_index > *dst_len) {
                return false;
            }
        } else {
            dst[write_index++] = src[read_index++];
            code++;
            if (code == 0xFF) {
                dst[code_index] = code;
                code_index      = write_index++;
                code            = 1;
                if (write_index > *dst_len) {
                    return false;
                }
            }
        }
        if (write_index > *dst_len) {
            return false;
        }
    }
    dst[code_index] = code;
    if (write_index >= *dst_len) {
        return false;
    }
    dst[write_index++] = 0; // Append the 0 byte at the end
    *dst_len           = write_index;
    return true;
}

//----------------------------------------------------------------------------------------------------------------------

bool cc_payload_cobs_decode(uint8_t *dst, size_t *dst_len, const uint8_t *src, size_t src_len)
{
    if (!dst || !dst_len || !src) {
        return false;
    }

    size_t read_index  = 0;
    size_t write_index = 0;

    // Assume the last byte (0) is always present and not part of the payload
    if (src_len == 0 || src[src_len - 1] != 0) {
        return false;
    }
    // Exclude the trailing zero from decoding
    size_t end = src_len - 1;

    while (read_index < end) {
        uint8_t code = src[read_index++];
        if (code == 0 || read_index + code - 1 > end + 1) {
            return false;
        }
        for (uint8_t i = 1; i < code; i++) {
            if (write_index >= *dst_len || read_index >= end) {
                return false;
            }
            dst[write_index++] = src[read_index++];
        }
        if (code != 0xFF && read_index < end) {
            if (write_index >= *dst_len) {
                return false;
            }
            dst[write_index++] = 0;
        }
    }
    *dst_len = write_index;
    return true;
}

//----------------------------------------------------------------------------------------------------------------------

void cc_checksum_update(uint8_t *checksum, uint8_t data)
{
    if (checksum == NULL) {
        return;
    }

    (*checksum) += data;
}

//----------------------------------------------------------------------------------------------------------------------

uint8_t cc_checksum_calculate(const uint8_t *data, size_t size)
{
    if (data == NULL || size == 0) {
        return CC_CHECKSUM_OK;
    }

    uint8_t checksum = 0;
    for (size_t i = 0; i < size; i++) {
        checksum += data[i];
    }
    return (uint8_t)(-checksum);
}

//----------------------------------------------------------------------------------------------------------------------

cc_action_t cc_header_action_get(cc_msg_header_t header)
{
    return (cc_action_t)(header.raw[0] >> 6);
}

//----------------------------------------------------------------------------------------------------------------------

void cc_header_action_set(cc_msg_header_t *header, cc_action_t action)
{
    assert(header != NULL);
    header->raw[0] = (header->raw[0] & 0b00111111) | (action << 6);
}

//----------------------------------------------------------------------------------------------------------------------

cc_prop_id_t cc_header_property_get(cc_msg_header_t header)
{
    uint16_t property_msb = header.raw[0] & 0b00001111;        /* 4 LSB bits are stored in byte 0 */
    uint16_t property_lsb = (header.raw[1] >> 6) & 0b00000011; /* 2 MSB bits are stored in byte 1 */
    return (cc_prop_id_t)((property_msb << 2) | property_lsb);
}

void cc_header_property_set(cc_msg_header_t *header, cc_prop_id_t property)
{
    assert(header != NULL);
    assert(property <= (1 << 6) - 1); /* 6 bits are used for property */

    uint16_t property_msb = (property >> 2) & 0b1111; /* 4 MSB bits to be set */
    uint16_t property_lsb = property & 0b11;          /* 2 LSB bits to be set */

    header->raw[0] = (header->raw[0] & 0b11110000) | (property_msb & 0b00001111); /* Set 4 LSB bits in byte 0 */
    header->raw[1] = (header->raw[1] & 0b00111111) | (property_lsb << 6);         /* Set 2 MSB bits in byte 1 */
}

//----------------------------------------------------------------------------------------------------------------------

uint16_t cc_header_node_cnt_get(cc_msg_header_t header)
{
    uint16_t node_cnt_lsb = header.raw[1] & 0b111111; /* 6 LSB bits are stored in byte 1 */
    uint16_t node_cnt_msb = header.raw[2] >> 1;       /* 7 MSB bits are stored in byte 2 */
    return (node_cnt_msb << 6) | node_cnt_lsb;
}

//----------------------------------------------------------------------------------------------------------------------

void cc_header_node_cnt_set(cc_msg_header_t *header, uint16_t node_cnt)
{
    assert(header != NULL);
    assert(node_cnt <= (1 << 13) - 1); /* 13 bits are used for node count */

    uint16_t node_cnt_lsb = node_cnt & 0b111111;         /* 6 LSB bits to be set */
    uint16_t node_cnt_msb = (node_cnt >> 6) & 0b1111111; /* 7 MSB bits to be set */

    header->raw[1] = (header->raw[1] & 0b11000000) | (node_cnt_lsb & 0b00111111); /* Set 6 LSB bits in byte 1 */
    header->raw[2] = (header->raw[2] & 0b00000001) | (node_cnt_msb << 1);         /* Set 7 MSB bits in byte 2 */
}

//----------------------------------------------------------------------------------------------------------------------

bool cc_header_parity_check(cc_msg_header_t header)
{
    uint8_t count = 0;
    for (size_t i = 0; i < sizeof(header.raw); i++) {
        uint8_t b = header.raw[i];
        while (b) {
            count += b & 1;
            b >>= 1;
        }
    }
    return !(bool)(count & 1);
}

//----------------------------------------------------------------------------------------------------------------------

void cc_header_parity_set(cc_msg_header_t *header, bool valid)
{
    header->raw[2] ^= (uint8_t)cc_header_parity_check(*header) ^ (uint8_t)valid;
}

//----------------------------------------------------------------------------------------------------------------------
