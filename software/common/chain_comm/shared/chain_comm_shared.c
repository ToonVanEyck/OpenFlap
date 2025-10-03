#include "chain_comm_shared.h"
#include <assert.h>
#include <string.h>

//======================================================================================================================
//                                                   GLOBAL VARIABLES
//======================================================================================================================

/** Maps a node state to a state string. */
#ifdef CC_STATE_STRINGS
static const char *state_names[] = {CC_NODE_STATE_GENERATOR(GENERATE_2ND_FIELD)};
#endif

/** Maps a node error to an error string. */
#ifdef CC_ERROR_STRINGS
static const char *error_strs[] = {CC_NODE_ERROR_GENERATOR(GENERATE_3RD_FIELD)};
#endif

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

bool cc_header_staging_bit_get(cc_msg_header_t header)
{
    return (bool)((header.raw[0] >> 5) & 0x01);
}

//----------------------------------------------------------------------------------------------------------------------

void cc_header_staging_bit_set(cc_msg_header_t *header, bool stage)
{
    assert(header != NULL);
    header->raw[0] = (header->raw[0] & 0b11011111) | (stage << 5);
}

//----------------------------------------------------------------------------------------------------------------------

cc_prop_id_t cc_header_property_get(cc_msg_header_t header)
{
    const uint16_t lsb_bits = 1; /* Number of LSB bits stored in byte 1 */
    const uint16_t msb_bits = 5; /* Number of MSB bits stored in byte 0 */

    uint16_t lsb = (header.raw[1] & ~(0xFF >> lsb_bits)) >> (8 - lsb_bits);
    uint16_t msb = (header.raw[0] & ~(0xFF << msb_bits)) << lsb_bits;
    return (cc_prop_id_t)(msb | lsb);
}

void cc_header_property_set(cc_msg_header_t *header, cc_prop_id_t property)
{
    const uint16_t lsb_bits = 1; /* Number of LSB bits stored in byte 1 */
    const uint16_t msb_bits = 5; /* Number of MSB bits stored in byte 0 */
    assert(property <= (1 << (lsb_bits + msb_bits)) - 1);
    assert(header != NULL);

    uint16_t lsb = property << (8 - lsb_bits);
    uint16_t msb = property >> lsb_bits;

    header->raw[1] = (header->raw[1] & (0xFF >> lsb_bits)) | lsb;
    header->raw[0] = (header->raw[0] & (0xFF << msb_bits)) | msb;
}

//----------------------------------------------------------------------------------------------------------------------

uint16_t cc_header_node_cnt_get(cc_msg_header_t header)
{
    const uint16_t lsb_bits = 6; /* Number of LSB bits stored in byte 1 */
    const uint16_t msb_bits = 7; /* Number of MSB bits stored in byte 2 */

    uint16_t lsb = (header.raw[1] & ~(0xFF << lsb_bits));
    uint16_t msb = ((header.raw[2] & ~(0xFF >> msb_bits)) >> (8 - msb_bits)) << lsb_bits;
    return (uint16_t)(msb | lsb);
}

//----------------------------------------------------------------------------------------------------------------------

void cc_header_node_cnt_set(cc_msg_header_t *header, uint16_t node_cnt)
{
    const uint16_t lsb_bits = 6; /* Number of LSB bits stored in byte 1 */
    const uint16_t msb_bits = 7; /* Number of MSB bits stored in byte 2 */
    assert(node_cnt <= (1 << (lsb_bits + msb_bits)) - 1);
    assert(header != NULL);

    uint16_t lsb = node_cnt & ~(0xFF << lsb_bits);
    uint16_t msb = (node_cnt >> lsb_bits) << (8 - msb_bits);

    header->raw[1] = ((header->raw[1] & (0xFF << lsb_bits)) | lsb);
    header->raw[2] = ((header->raw[2] & (0xFF >> msb_bits)) | msb);
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

cc_sync_type_t cc_header_sync_type_get(cc_msg_header_t header)
{
    /* Byte 0 Bit 4-5 */
    return (cc_sync_type_t)((header.raw[0] >> 4) & 0x03);
}

//----------------------------------------------------------------------------------------------------------------------

void cc_header_sync_type_set(cc_msg_header_t *header, cc_sync_type_t sync_type)
{
    /* Byte 0 Bit 4-5 */
    assert(header != NULL);
    assert(sync_type <= 3);
    header->raw[0] = (header->raw[0] & 0b11001111) | (sync_type << 4);
}

//----------------------------------------------------------------------------------------------------------------------

cc_sync_error_code_t cc_header_sync_error_get(cc_msg_header_t header)
{
    /* Byte 0 Bit 0-3 */
    return (cc_sync_error_code_t)(header.raw[0] & 0x0F);
}

//----------------------------------------------------------------------------------------------------------------------

void cc_header_sync_error_set(cc_msg_header_t *header, cc_sync_error_code_t error_code)
{
    /* Byte 0 Bit 0-3 */
    assert(header != NULL);
    assert(error_code <= 0x0F);
    header->raw[0] = (header->raw[0] & 0xF0) | (error_code & 0x0F);
}

//----------------------------------------------------------------------------------------------------------------------

void cc_header_sync_error_add(cc_msg_header_t *header, cc_sync_error_code_t error_code)
{
    /* Byte 0 Bit 0-3 */
    assert(header != NULL);
    assert(error_code <= 0x0F);
    header->raw[0] |= (error_code & 0x0F);
}

//----------------------------------------------------------------------------------------------------------------------

char *cc_node_state_to_str(cc_node_state_t state)
{
#ifdef CC_STATE_STRINGS
    return (char *)state_names[state];
#else
    return "?";
#endif
}

//----------------------------------------------------------------------------------------------------------------------

char *cc_node_error_to_str(cc_node_err_t error)
{
#ifdef CC_ERROR_STRINGS
    return (char *)error_strs[error];
#else
    return "?";
#endif
}