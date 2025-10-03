#include "chain_comm_master_test.h"
#include "chain_comm_node_test.h"
#include "test_properties.h"

#include "unity.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

//======================================================================================================================
//                                                   MACROS a DEFINES
//======================================================================================================================

#define TEST_ASSERT_COBS_ENCODED(_encoded_data, _encoded_data_len)                                                     \
    do {                                                                                                               \
        for (size_t i = 0; i < _encoded_data_len - 1; i++) {                                                           \
            TEST_ASSERT_NOT_EQUAL_MESSAGE(0, _encoded_data[i],                                                         \
                                          "COBS encoding failed, zero byte found in encoded data");                    \
        }                                                                                                              \
        TEST_ASSERT_EQUAL_MESSAGE(0, _encoded_data[_encoded_data_len - 1],                                             \
                                  "COBS encoding failed, last byte is not zero");                                      \
    } while (0);

//======================================================================================================================
//                                                   TYPE DEFINITIONS
//======================================================================================================================

typedef struct {
    uint8_t rx_buf[1000];
    uint8_t tx_buf[1000];
    size_t rx_cnt;
    size_t tx_cnt;
    bool is_busy;
    bool tx_busy;
    size_t cnt_readable;
    size_t cnt_writable;
} uart_dummy_ctx_t;

typedef struct {
    uint8_t data[1000];
    bool success;
    size_t size;
} prop_dummy_ctx_t;

//======================================================================================================================
//                                                   FUNCTION PROTOTYPES
//======================================================================================================================

size_t uart_dummy_read(void *uart_userdata, uint8_t *data, size_t size);
size_t uart_dummy_cnt_readable(void *uart_userdata);
size_t uart_dummy_write(void *uart_userdata, const uint8_t *data, size_t size);
size_t uart_dummy_cnt_writable(void *uart_userdata);
bool uart_dummy_tx_buff_empty(void *uart_userdata);
bool uart_dummy_is_busy(void *uart_userdata);
void uart_dummy_flush_rx_buff(void *uart_userdata);
bool prop_dummy_set_handler(void *userdata, uint16_t node_idx, uint8_t *buf, size_t *size);
bool prop_dummy_get_handler(void *userdata, uint16_t node_idx, uint8_t *buf, size_t *size);
void randomize_array(uint8_t *array, size_t size);
void node_dummy_config(cc_node_ctx_t *node_ctx);

//======================================================================================================================
//                                                   GLOBAL VARIABLES
//======================================================================================================================

static uart_dummy_ctx_t uart_ctx = {0};
static prop_dummy_ctx_t prop_ctx = {0};
static cc_prop_t prop_list[1]    = {
    {
           .attribute = {.name = "Dummy Property"},
           .handler   = {.set = prop_dummy_set_handler, .get = prop_dummy_get_handler},
    },
};

//======================================================================================================================
//                                                   TEST FUNCTIONS
//======================================================================================================================

void setUp(void)
{
    // This function is called before each test
}

//----------------------------------------------------------------------------------------------------------------------

void tearDown(void)
{
    // This function is called after each test
}

void test_cc_cobs_encode_decode_ok(void)
{
    srand(0xdeadbeef); // Seed for reproducibility
    uint8_t payload_in[1000] = {0}, payload_out[1000] = {0}, encoded[1000] = {0};
    size_t payload_in_len = 0, payload_out_len = 0, encoded_len = 0;
    randomize_array(payload_in, 1000); /* Randomize input payload. */
    payload_in[10] = 0;                /* Insert some a zero bytes to test COBS. */
    payload_in[50] = 0;

    /* Normal test case. */
    payload_in_len  = 100;
    encoded_len     = sizeof(encoded);     /* "unlimited" */
    payload_out_len = sizeof(payload_out); /* "unlimited" */
    TEST_ASSERT_TRUE(cc_payload_cobs_encode(encoded, &encoded_len, payload_in, payload_in_len));
    TEST_ASSERT_TRUE(cc_payload_cobs_decode(payload_out, &payload_out_len, encoded, encoded_len));
    TEST_ASSERT_EQUAL(payload_in_len, payload_out_len);
    TEST_ASSERT_COBS_ENCODED(encoded, encoded_len);

    /* Long test case. */
    payload_in_len = 500;
    memset(payload_in, 0xFF, 300); /* Ensure more than 254 non zero */
    payload_in[301] = 0;
    encoded_len     = sizeof(encoded);     /* "unlimited" */
    payload_out_len = sizeof(payload_out); /* "unlimited" */
    TEST_ASSERT_TRUE(cc_payload_cobs_encode(encoded, &encoded_len, payload_in, payload_in_len));
    TEST_ASSERT_TRUE(cc_payload_cobs_decode(payload_out, &payload_out_len, encoded, encoded_len));
    TEST_ASSERT_EQUAL(payload_in_len, payload_out_len);
    TEST_ASSERT_COBS_ENCODED(encoded, encoded_len);

    /* Decode empty payload. */
    encoded[0]      = 0;
    encoded_len     = 1;
    payload_out_len = sizeof(payload_out); /* "unlimited" */
    TEST_ASSERT_TRUE(cc_payload_cobs_decode(payload_out, &payload_out_len, encoded, encoded_len));
    TEST_ASSERT_EQUAL(payload_out_len, 0);
}

//----------------------------------------------------------------------------------------------------------------------

void test_cc_cobs_encode_nok(void)
{
    srand(0xdeadbeef); // Seed for reproducibility
    uint8_t payload_in[1000] = {0}, encoded[1000] = {0};
    size_t payload_in_len = 0, encoded_len = 0;
    randomize_array(payload_in, 1000); /* Randomize input payload. */
    payload_in[10] = 0;                /* Insert some a zero bytes to test COBS. */
    payload_in[50] = 0;

    /* Invalid arguments. */
    TEST_ASSERT_FALSE(cc_payload_cobs_encode(NULL, &encoded_len, payload_in, payload_in_len));
    TEST_ASSERT_FALSE(cc_payload_cobs_encode(encoded, NULL, payload_in, payload_in_len));
    TEST_ASSERT_FALSE(cc_payload_cobs_encode(encoded, &encoded_len, NULL, payload_in_len));

    /* Not enough space to encode message. */
    payload_in_len = 100;
    encoded_len    = 10; /* Not enough space. */
    TEST_ASSERT_FALSE(cc_payload_cobs_encode(encoded, &encoded_len, payload_in, payload_in_len));
    TEST_ASSERT_EQUAL(10, encoded_len); /* Length should not have changed. */

    /* Not enough space to append final 0. */
    memset(payload_in, 0xAA, 10); /* 10 non-zero bytes */
    payload_in[0]  = 0;
    payload_in_len = 10;
    encoded_len    = 11; /* Not enough space. */
    TEST_ASSERT_FALSE(cc_payload_cobs_encode(encoded, &encoded_len, payload_in, payload_in_len));

    /* Not enough space to append RLE byte. */
    memset(payload_in, 0, 10); /* 10 non-zero bytes */
    payload_in[0]  = 0;
    payload_in_len = 10;
    encoded_len    = 10; /* Not enough space. */
    TEST_ASSERT_FALSE(cc_payload_cobs_encode(encoded, &encoded_len, payload_in, payload_in_len));

    /* Not enough space to append second RLE byte. */
    memset(payload_in, 0xAA, 255); /* 255 non-zero bytes */
    payload_in[0]  = 0;
    payload_in_len = 255;
    encoded_len    = 256; /* Not enough space. */
    TEST_ASSERT_FALSE(cc_payload_cobs_encode(encoded, &encoded_len, payload_in, payload_in_len));
}

//----------------------------------------------------------------------------------------------------------------------

void test_cc_cobs_decode_nok(void)
{
    srand(0xdeadbeef); // Seed for reproducibility
    uint8_t payload_out[1000] = {0}, encoded[1000] = {0};
    size_t payload_out_len = 0, encoded_len = 0;

    /* Invalid arguments. */
    TEST_ASSERT_FALSE(cc_payload_cobs_decode(NULL, &payload_out_len, encoded, encoded_len));
    TEST_ASSERT_FALSE(cc_payload_cobs_decode(payload_out, NULL, encoded, encoded_len));
    TEST_ASSERT_FALSE(cc_payload_cobs_decode(payload_out, &payload_out_len, NULL, encoded_len));
    TEST_ASSERT_FALSE(cc_payload_cobs_decode(payload_out, &payload_out_len, NULL, 0));

    /* Encoding not 0 terminated. */
    payload_out_len = sizeof(payload_out); /* "unlimited" */
    encoded_len     = 4;
    memcpy(encoded, (uint8_t[]) {3, 1, 2, 3}, encoded_len); /* Encoded version of {1,2,3} without trailing 0 */
    TEST_ASSERT_FALSE(cc_payload_cobs_decode(payload_out, &payload_out_len, encoded, encoded_len));
    TEST_ASSERT_EQUAL(sizeof(payload_out), payload_out_len); /* Length should not have changed. */

    /* Unexpected 0 in encoding. */
    payload_out_len = sizeof(payload_out); /* "unlimited" */
    encoded_len     = 12;
    memcpy(encoded, (uint8_t[]) {21, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 0},
           encoded_len); /* Encoded version of {1..10}, but initial byte says 0 is expected after 21 bytes not 11. */
    TEST_ASSERT_FALSE(cc_payload_cobs_decode(payload_out, &payload_out_len, encoded, encoded_len));

    /* Not enough space to decode message. */
    payload_out_len = 10; /* Not enough space. */
    encoded_len     = 22;
    memcpy(encoded, (uint8_t[]) {21, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 0},
           encoded_len); /* Encoded version of {1..20} */
    TEST_ASSERT_FALSE(cc_payload_cobs_decode(payload_out, &payload_out_len, encoded, encoded_len));
    TEST_ASSERT_EQUAL(10, payload_out_len);

    /* Not enough space to decode message with zero's. */
    payload_out_len = 5; /* Not enough space. */
    encoded_len     = 8;
    memcpy(encoded, (uint8_t[]) {1, 1, 1, 1, 1, 1, 1, 0}, encoded_len); /* Encoded version of {0,0,0,0,0,0} */
    TEST_ASSERT_FALSE(cc_payload_cobs_decode(payload_out, &payload_out_len, encoded, encoded_len));
    TEST_ASSERT_EQUAL(5, payload_out_len);
}

//----------------------------------------------------------------------------------------------------------------------

void test_cc_checksum_calculate(void)
{
    srand(0xdeadbeef); // Seed for reproducibility
    uint8_t checksum  = 0;
    uint8_t data[256] = {0};
    uint8_t len       = 0;
    for (int i = 0; i < 25; i++) {
        len = (uint8_t)(rand() % 0xFF);
        randomize_array(data, len);
        checksum = cc_checksum_calculate(data, len);
        TEST_ASSERT_NOT_EQUAL(CC_CHECKSUM_OK, checksum); /* Random data should not give valid checksum. */
        data[len] = (uint8_t)(checksum);                 /* Add checksum byte. */
        checksum  = cc_checksum_calculate(data, len + 1);
        TEST_ASSERT_EQUAL(CC_CHECKSUM_OK, checksum); /* Data should be good now. */
    }

    /* Test invalid arg cases: */
    TEST_ASSERT_EQUAL(CC_CHECKSUM_OK, cc_checksum_calculate(NULL, 0));
    TEST_ASSERT_EQUAL(CC_CHECKSUM_OK, cc_checksum_calculate(data, 0));
}

//----------------------------------------------------------------------------------------------------------------------

void test_cc_checksum_update(void)
{
    uint8_t checksum = 0;

    /* No checksum provided. */
    cc_checksum_update(NULL, 0); /* Should not crash */

    /* Basic test case. */
    checksum = 0;
    cc_checksum_update(&checksum, 0);
    TEST_ASSERT_EQUAL(0, checksum);
    cc_checksum_update(&checksum, 1);
    TEST_ASSERT_EQUAL(1, checksum);
    cc_checksum_update(&checksum, 2);
    TEST_ASSERT_EQUAL(3, checksum);
    cc_checksum_update(&checksum, 3);
    TEST_ASSERT_EQUAL(6, checksum);
    cc_checksum_update(&checksum, 255);
    TEST_ASSERT_EQUAL(5, checksum); /* Overflow */
}

//----------------------------------------------------------------------------------------------------------------------

void test_cc_parity_check(void)
{
    cc_msg_header_t header = {0};

    /* Parity Ok */
    memcpy(header.raw, (uint8_t[3]) {0b00000000, 0b00000000, 0b00000000}, sizeof(header.raw));
    TEST_ASSERT_TRUE(cc_header_parity_check(header)); // All zeros -> even parity

    memcpy(header.raw, (uint8_t[3]) {0b11111111, 0b11111111, 0b11111111}, sizeof(header.raw));
    TEST_ASSERT_TRUE(cc_header_parity_check(header)); // All ones -> even parity

    memcpy(header.raw, (uint8_t[3]) {0b01010001, 0b00011000, 0b10000000}, sizeof(header.raw));
    TEST_ASSERT_TRUE(cc_header_parity_check(header)); // Mixed bits -> even parity

    /* Parity Error */
    memcpy(header.raw, (uint8_t[3]) {0b00000000, 0b00000000, 0b00000001}, sizeof(header.raw));
    TEST_ASSERT_FALSE(cc_header_parity_check(header)); // One bit set -> odd parity

    memcpy(header.raw, (uint8_t[3]) {0b11111111, 0b11111111, 0b11111110}, sizeof(header.raw));
    TEST_ASSERT_FALSE(cc_header_parity_check(header)); // One bit cleared -> odd parity

    memcpy(header.raw, (uint8_t[3]) {0b01100001, 0b00011000, 0b11000000}, sizeof(header.raw));
    TEST_ASSERT_FALSE(cc_header_parity_check(header)); // Mixed bits -> odd parity
}

//--------------------------------------------------------------------------------------------------------------------------------------

void test_cc_header_action_set_get(void)
{
    cc_msg_header_t header = {0};

    cc_header_action_set(&header, CC_ACTION_READ);
    TEST_ASSERT_EQUAL(0b00000000, header.raw[0]);
    TEST_ASSERT_EQUAL(0, header.raw[1]);
    TEST_ASSERT_EQUAL(0, header.raw[2]);
    TEST_ASSERT_EQUAL(CC_ACTION_READ, cc_header_action_get(header));

    cc_header_action_set(&header, CC_ACTION_WRITE);
    TEST_ASSERT_EQUAL(0b01000000, header.raw[0]);
    TEST_ASSERT_EQUAL(0, header.raw[1]);
    TEST_ASSERT_EQUAL(0, header.raw[2]);
    TEST_ASSERT_EQUAL(CC_ACTION_WRITE, cc_header_action_get(header));

    cc_header_action_set(&header, CC_ACTION_BROADCAST);
    TEST_ASSERT_EQUAL(0b10000000, header.raw[0]);
    TEST_ASSERT_EQUAL(0, header.raw[1]);
    TEST_ASSERT_EQUAL(0, header.raw[2]);
    TEST_ASSERT_EQUAL(CC_ACTION_BROADCAST, cc_header_action_get(header));

    cc_header_action_set(&header, CC_ACTION_SYNC);
    TEST_ASSERT_EQUAL(0b11000000, header.raw[0]);
    TEST_ASSERT_EQUAL(0, header.raw[1]);
    TEST_ASSERT_EQUAL(0, header.raw[2]);
    TEST_ASSERT_EQUAL(CC_ACTION_SYNC, cc_header_action_get(header));
}

//----------------------------------------------------------------------------------------------------------------------

void test_cc_header_staging_bit_set_get(void)
{
    /* Read error bit is mapped to the same bit as staging bit. */
    cc_msg_header_t header = {0};

    cc_header_staging_bit_set(&header, true);
    TEST_ASSERT_EQUAL(0b00100000, header.raw[0]);
    TEST_ASSERT_EQUAL(0, header.raw[1]);
    TEST_ASSERT_EQUAL(0, header.raw[2]);
    TEST_ASSERT_TRUE(cc_header_staging_bit_get(header));
    TEST_ASSERT_TRUE(cc_header_read_error_bit_get(header));

    cc_header_read_error_bit_set(&header, false);
    TEST_ASSERT_EQUAL(0b00000000, header.raw[0]);
    TEST_ASSERT_EQUAL(0, header.raw[1]);
    TEST_ASSERT_EQUAL(0, header.raw[2]);
    TEST_ASSERT_FALSE(cc_header_staging_bit_get(header));
}

//--------------------------------------------------------------------------------------------------------------------------------------

void test_cc_header_property_set_get(void)
{
    cc_msg_header_t header = {0};

    cc_header_property_set(&header, 0b10); /* This should only affect raw byte [0]. */
    TEST_ASSERT_EQUAL(0b00000001, header.raw[0]);
    TEST_ASSERT_EQUAL(0b00000000, header.raw[1]);
    TEST_ASSERT_EQUAL(0b00000000, header.raw[2]);
    TEST_ASSERT_EQUAL(0b10, cc_header_property_get(header));

    cc_header_property_set(&header, 0b01); /* This should only affect raw byte [1]. */
    TEST_ASSERT_EQUAL(0b00000000, header.raw[0]);
    TEST_ASSERT_EQUAL(0b10000000, header.raw[1]);
    TEST_ASSERT_EQUAL(0b00000000, header.raw[2]);
    TEST_ASSERT_EQUAL(0b01, cc_header_property_get(header));

    cc_header_property_set(&header, 0b111111); /* All property bits set. */
    TEST_ASSERT_EQUAL(0b00011111, header.raw[0]);
    TEST_ASSERT_EQUAL(0b10000000, header.raw[1]);
    TEST_ASSERT_EQUAL(0b00000000, header.raw[2]);
    TEST_ASSERT_EQUAL(0b111111, cc_header_property_get(header));

    memset(header.raw, 0xFF, sizeof(header.raw));
    cc_header_property_set(&header, 0); /* Clear property */
    TEST_ASSERT_EQUAL(0b11100000, header.raw[0]);
    TEST_ASSERT_EQUAL(0b01111111, header.raw[1]);
    TEST_ASSERT_EQUAL(0b11111111, header.raw[2]);
}

//----------------------------------------------------------------------------------------------------------------------

void test_cc_node_cnt_set_get(void)
{
    cc_msg_header_t header = {0};

    cc_header_node_cnt_set(&header, 0b111111); /* This should only affect raw byte [1]*/
    TEST_ASSERT_EQUAL(0b00000000, header.raw[0]);
    TEST_ASSERT_EQUAL(0b00111111, header.raw[1]);
    TEST_ASSERT_EQUAL(0b00000000, header.raw[2]);

    uint16_t node_cnt = cc_header_node_cnt_get(header);

    TEST_ASSERT_EQUAL(0b111111, node_cnt);
    node_cnt++;
    cc_header_node_cnt_set(&header, node_cnt); /* This should only affect raw byte [0]*/
    TEST_ASSERT_EQUAL(0b00000000, header.raw[0]);
    TEST_ASSERT_EQUAL(0b00000000, header.raw[1]);
    TEST_ASSERT_EQUAL(0b00000010, header.raw[2]);
    node_cnt = cc_header_node_cnt_get(header);
    TEST_ASSERT_EQUAL(0b1000000, node_cnt); /* 64 */

    memset(header.raw, 0xFF, sizeof(header.raw));
    cc_header_node_cnt_set(&header, 0); /* Clear node count */
    TEST_ASSERT_EQUAL(0b11111111, header.raw[0]);
    TEST_ASSERT_EQUAL(0b11000000, header.raw[1]);
    TEST_ASSERT_EQUAL(0b00000001, header.raw[2]);
}

//----------------------------------------------------------------------------------------------------------------------

void test_cc_header_parity_set_get(void)
{
    /* Configure two identical headers. */
    cc_msg_header_t header_original = {.raw = {0b01100001, 0b00011000, 0b11000000}};
    cc_msg_header_t header_fixed    = {.raw = {0b01100001, 0b00011000, 0b11000000}};

    /* Check that parity is initially wrong. */
    TEST_ASSERT_FALSE(cc_header_parity_check(header_original));

    /* Fix parity. */
    cc_header_parity_set(&header_fixed, true);

    /* Check that parity is now correct. */
    TEST_ASSERT_TRUE(cc_header_parity_check(header_fixed));

    TEST_ASSERT_EQUAL(header_original.raw[0], header_fixed.raw[0]);
    TEST_ASSERT_EQUAL(header_original.raw[1], header_fixed.raw[1]);
    TEST_ASSERT_EQUAL(header_original.raw[2] & 0b11111110, header_fixed.raw[2] & 0b11111110);

    /* Check that only the parity bit was changed. */
    TEST_ASSERT_TRUE((header_original.raw[2] & 1) != (header_fixed.raw[2] & 1));

    /* Break parity. */
    cc_header_parity_set(&header_fixed, false);

    /* Check that parity is now incorrect. */
    TEST_ASSERT_FALSE(cc_header_parity_check(header_fixed));

    /* Additional test case. */
    header_original = (cc_msg_header_t) {.raw = {0x00, 0x03, 0x01}};
    cc_header_parity_set(&header_original, true);
    TEST_ASSERT_TRUE(cc_header_parity_check(header_original)); // Parity should be ok.
}

//----------------------------------------------------------------------------------------------------------------------

void test_cc_header_sync_type_set_get(void)
{
    cc_msg_header_t header = {0};

    cc_header_sync_type_set(&header, CC_SYNC_ACK);
    TEST_ASSERT_EQUAL(0b00000000, header.raw[0]);
    TEST_ASSERT_EQUAL(0, header.raw[1]);
    TEST_ASSERT_EQUAL(0, header.raw[2]);
    TEST_ASSERT_EQUAL(CC_SYNC_ACK, cc_header_sync_type_get(header));

    cc_header_sync_type_set(&header, CC_SYNC_COMMIT);
    TEST_ASSERT_EQUAL(0b00010000, header.raw[0]);
    TEST_ASSERT_EQUAL(0, header.raw[1]);
    TEST_ASSERT_EQUAL(0, header.raw[2]);
    TEST_ASSERT_EQUAL(CC_SYNC_COMMIT, cc_header_sync_type_get(header));

    header.raw[0] = 0xFF; // Set all bits
    cc_header_sync_type_set(&header, CC_SYNC_ACK);
    TEST_ASSERT_EQUAL(0b11001111, header.raw[0]);
    TEST_ASSERT_EQUAL(0, header.raw[1]);
    TEST_ASSERT_EQUAL(0, header.raw[2]);
    TEST_ASSERT_EQUAL(CC_SYNC_ACK, cc_header_sync_type_get(header));

    header.raw[0] = 0xFF; // Set all bits
    cc_header_sync_type_set(&header, CC_SYNC_COMMIT);
    TEST_ASSERT_EQUAL(0b11011111, header.raw[0]);
    TEST_ASSERT_EQUAL(0, header.raw[1]);
    TEST_ASSERT_EQUAL(0, header.raw[2]);
    TEST_ASSERT_EQUAL(CC_SYNC_COMMIT, cc_header_sync_type_get(header));
}

//----------------------------------------------------------------------------------------------------------------------

void test_cc_header_sync_error_set_get(void)
{
    cc_msg_header_t header = {0};

    header.raw[0] = 0xFF; // Set all bits
    cc_header_sync_error_set(&header, CC_SYNC_ERR_NONE);
    TEST_ASSERT_EQUAL(0b11110000, header.raw[0]);
    TEST_ASSERT_EQUAL(0, header.raw[1]);
    TEST_ASSERT_EQUAL(0, header.raw[2]);
    TEST_ASSERT_EQUAL(CC_SYNC_ERR_NONE, cc_header_sync_error_get(header));

    cc_header_sync_error_add(&header, CC_SYNC_ERR_OTHER);
    TEST_ASSERT_EQUAL(0b11110001, header.raw[0]);
    TEST_ASSERT_EQUAL(0, header.raw[1]);
    TEST_ASSERT_EQUAL(0, header.raw[2]);
    TEST_ASSERT_EQUAL(CC_SYNC_ERR_OTHER, cc_header_sync_error_get(header));

    cc_header_sync_error_add(&header, CC_SYNC_ERR_TRANSMISSION);
    TEST_ASSERT_EQUAL(0b11110011, header.raw[0]);
    TEST_ASSERT_EQUAL(0, header.raw[1]);
    TEST_ASSERT_EQUAL(0, header.raw[2]);
    TEST_ASSERT_EQUAL(CC_SYNC_ERR_OTHER | CC_SYNC_ERR_TRANSMISSION, cc_header_sync_error_get(header));

    cc_header_sync_error_add(&header, CC_SYNC_ERR_HANDLER);
    TEST_ASSERT_EQUAL(0b11110111, header.raw[0]);
    TEST_ASSERT_EQUAL(0, header.raw[1]);
    TEST_ASSERT_EQUAL(0, header.raw[2]);
    TEST_ASSERT_EQUAL(CC_SYNC_ERR_OTHER | CC_SYNC_ERR_TRANSMISSION | CC_SYNC_ERR_HANDLER,
                      cc_header_sync_error_get(header));

    cc_header_sync_error_add(&header, CC_SYNC_ERR_NOT_SUPPORTED);
    TEST_ASSERT_EQUAL(0b11111111, header.raw[0]);
    TEST_ASSERT_EQUAL(0, header.raw[1]);
    TEST_ASSERT_EQUAL(0, header.raw[2]);
    TEST_ASSERT_EQUAL(CC_SYNC_ERR_OTHER | CC_SYNC_ERR_TRANSMISSION | CC_SYNC_ERR_HANDLER | CC_SYNC_ERR_NOT_SUPPORTED,
                      cc_header_sync_error_get(header));
}

//----------------------------------------------------------------------------------------------------------------------

void test_cc_master_prop_read(void)
{
    /* Initialize a test master. */
    cc_test_master_ctx_t test_master_ctx;
    cc_test_master_init(&test_master_ctx);

    int pipefd[2];
    if (pipe(pipefd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    test_master_ctx.uart.rx_fd = pipefd[0];
    uart_driver_t test_node    = {.rx_fd = test_master_ctx.original_rx_fd, .tx_fd = pipefd[1], .print_debug = "NODE"};

    uint8_t expected_data[5] = {0};
    memset(expected_data, 0xAA, sizeof(expected_data) - 1);
    expected_data[sizeof(expected_data) - 1] = cc_checksum_calculate(expected_data, sizeof(expected_data) - 1);

    uint8_t tx_payload[100] = {0};
    size_t tx_payload_len   = sizeof(tx_payload) - 3;

    cc_msg_header_t header = {0};
    cc_header_action_set(&header, CC_ACTION_READ);
    cc_header_property_set(&header, PROP_STATIC_RW);
    cc_header_node_cnt_set(&header, 1);
    cc_header_parity_set(&header, true);

    tx_payload[0] = header.raw[0];
    tx_payload[1] = header.raw[1];
    tx_payload[2] = header.raw[2];
    cc_payload_cobs_encode(tx_payload + 3, &tx_payload_len, expected_data, sizeof(expected_data));

    uart_write(&test_node, tx_payload, tx_payload_len + 3);

    /* Try a read all command */
    cc_master_prop_read(&test_master_ctx.master_ctx, PROP_STATIC_RW);

    uint8_t received_data[100] = {0};
    uart_read(&test_master_ctx.uart, received_data, 3);
}

//----------------------------------------------------------------------------------------------------------------------

void test_master_read_nodes(void)
{
    size_t node_cnt     = 10;
    cc_master_err_t err = CC_MASTER_OK;

    /* Initialize master. */
    cc_test_master_ctx_t test_master_ctx = {0};
    cc_test_master_init(&test_master_ctx);

    TEST_ASSERT_NULL(test_master_ctx.node_data);
    TEST_ASSERT_EQUAL(0, test_master_ctx.node_cnt);

    /* Initialize the nodes. */
    cc_test_node_group_ctx_t node_test_grp = {0};
    setup_cc_node_prop_list_handlers();
    cc_test_node_init(&node_test_grp, node_cnt, &test_master_ctx);

    /* Try a read all command */
    for (int i = 0; i < node_cnt; i++) {
        // randomize_array(node_test_grp.node_list[i].node_data, TEST_PROP_SIZE);
        memset(node_test_grp.node_list[i].node_data, 0xAA, TEST_PROP_SIZE);
    }

    err = cc_master_prop_read(&test_master_ctx.master_ctx, PROP_STATIC_RW);
    usleep(10000);
    TEST_ASSERT_EQUAL(CC_MASTER_OK, err);

    TEST_ASSERT_NOT_NULL(test_master_ctx.node_data);
    TEST_ASSERT_EQUAL(node_cnt, test_master_ctx.node_cnt);

    for (int i = 0; i < node_cnt; i++) {
        TEST_ASSERT_EQUAL_HEX8_ARRAY(test_master_ctx.node_data + (i * TEST_PROP_SIZE),
                                     node_test_grp.node_list[i].node_data, TEST_PROP_SIZE);
        TEST_ASSERT_EQUAL(CC_NODE_STATE_RX_HEADER, node_test_grp.node_list[i].node_ctx.state);
    }

    /* Deinitialize master and nodes. */
    cc_test_master_deinit(&test_master_ctx);
    cc_test_node_deinit(&node_test_grp);
}

//----------------------------------------------------------------------------------------------------------------------

void test_master_write_nodes(void)
{
    size_t node_cnt     = 5;
    cc_master_err_t err = CC_MASTER_OK;

    /* Initialize master. */
    cc_test_master_ctx_t test_master_ctx = {0};
    cc_test_master_init(&test_master_ctx);
    test_master_ctx.node_data = malloc(TEST_PROP_SIZE * node_cnt);
    memset(test_master_ctx.node_data, 0xAA, TEST_PROP_SIZE * node_cnt);
    test_master_ctx.node_cnt = node_cnt;

    TEST_ASSERT_NOT_NULL(test_master_ctx.node_data);
    TEST_ASSERT_EQUAL(node_cnt, test_master_ctx.node_cnt);

    /* Initialize the nodes. */
    cc_test_node_group_ctx_t node_test_grp = {0};
    setup_cc_node_prop_list_handlers();
    cc_test_node_init(&node_test_grp, node_cnt, &test_master_ctx);

    /* Try a read all command */
    for (int i = 0; i < node_cnt; i++) {
        // randomize_array(node_test_grp.node_list[i].node_data, TEST_PROP_SIZE);
        memset(node_test_grp.node_list[i].node_data, 0x00, TEST_PROP_SIZE);
    }

    err = cc_master_prop_write(&test_master_ctx.master_ctx, PROP_STATIC_RW, 0, false, true);
    usleep(10000);
    TEST_ASSERT_EQUAL(CC_MASTER_OK, err);

    TEST_ASSERT_NOT_NULL(test_master_ctx.node_data);
    TEST_ASSERT_EQUAL(node_cnt, test_master_ctx.node_cnt);

    for (int i = 0; i < node_cnt; i++) {
        TEST_ASSERT_EQUAL_HEX8_ARRAY(test_master_ctx.node_data + (i * TEST_PROP_SIZE),
                                     node_test_grp.node_list[i].node_data, TEST_PROP_SIZE);
        TEST_ASSERT_EQUAL(CC_NODE_STATE_RX_HEADER, node_test_grp.node_list[i].node_ctx.state);
    }

    /* Deinitialize master and nodes. */
    cc_test_master_deinit(&test_master_ctx);
    cc_test_node_deinit(&node_test_grp);
}

//----------------------------------------------------------------------------------------------------------------------

void test_node_init(void)
{
    cc_node_ctx_t node_ctx = {0};

    cc_node_uart_cb_cfg_t uart_cb = (cc_node_uart_cb_cfg_t) {
        .read          = uart_dummy_read,
        .cnt_readable  = uart_dummy_cnt_readable,
        .write         = uart_dummy_write,
        .cnt_writable  = uart_dummy_cnt_writable,
        .tx_buff_empty = uart_dummy_tx_buff_empty,
        .is_busy       = uart_dummy_is_busy,
    };
    void *uart_userdata   = (void *)0xabad1dea;      // Dummy pointer
    cc_prop_t *prop_list  = (cc_prop_t *)0xfeedface; // Dummy pointer
    void *prop_userdata   = (void *)0xdefec8ed;      // Dummy pointer
    size_t prop_list_size = 10;

    cc_node_init(&node_ctx, &uart_cb, uart_userdata, prop_list, prop_list_size, prop_userdata);

    TEST_ASSERT_EQUAL(prop_list, node_ctx.prop_list);
    TEST_ASSERT_EQUAL(prop_list_size, node_ctx.prop_list_size);
    TEST_ASSERT_EQUAL(prop_userdata, node_ctx.prop_userdata);
    TEST_ASSERT_EQUAL(uart_cb.read, node_ctx.uart.read);
    TEST_ASSERT_EQUAL(uart_cb.cnt_readable, node_ctx.uart.cnt_readable);
    TEST_ASSERT_EQUAL(uart_cb.write, node_ctx.uart.write);
    TEST_ASSERT_EQUAL(uart_cb.cnt_writable, node_ctx.uart.cnt_writable);
    TEST_ASSERT_EQUAL(uart_cb.tx_buff_empty, node_ctx.uart.tx_buff_empty);
    TEST_ASSERT_EQUAL(uart_cb.is_busy, node_ctx.uart.is_busy);
    TEST_ASSERT_EQUAL(uart_userdata, node_ctx.uart_userdata);

    TEST_ASSERT_EQUAL(CC_NODE_STATE_RX_HEADER, node_ctx.state);
    TEST_ASSERT_EQUAL(CC_NODE_STATE_UNDEFINED, node_ctx.next_state);
    TEST_ASSERT_EQUAL(CC_NODE_ERR_NONE, node_ctx.last_error);
    TEST_ASSERT_EQUAL(CC_NODE_STATE_UNDEFINED, node_ctx.error_state);

    TEST_ASSERT_EQUAL(0, node_ctx.header.raw[0]);
    TEST_ASSERT_EQUAL(0, node_ctx.header.raw[1]);
    TEST_ASSERT_EQUAL(0, node_ctx.header.raw[2]);
    TEST_ASSERT_EQUAL(0, node_ctx.action);
    TEST_ASSERT_EQUAL(0, node_ctx.staged_write);
    TEST_ASSERT_EQUAL(0, node_ctx.property_id);
    TEST_ASSERT_EQUAL(0, node_ctx.node_cnt);
    TEST_ASSERT_EQUAL(0, node_ctx.data_cnt);
    TEST_ASSERT_EQUAL(0, node_ctx.property_data[0]);
    TEST_ASSERT_EQUAL(0, node_ctx.property_size);
}

//----------------------------------------------------------------------------------------------------------------------

void test_node_rx_header_state_read_action_ok(void)
{
    /* Test Params: */
    cc_action_t test_action   = CC_ACTION_READ;
    cc_prop_id_t test_prop_id = 0;    /* Only one handler so must be zero!*/
    uint16_t test_node_cnt    = 0xFF; /* This value cause a carry from raw[1] to raw[2] */

    /* Setup a node. */
    cc_node_ctx_t node_ctx = {0};
    node_dummy_config(&node_ctx);

    /* Setup the state. */
    node_ctx.state      = CC_NODE_STATE_RX_HEADER;
    node_ctx.next_state = CC_NODE_STATE_UNDEFINED;

    /* Run the chain communication node. */
    cc_node_tick(&node_ctx, 0); /* Initial Tick. */

    /* Write a dummy header. */
    cc_msg_header_t header = {0};
    cc_header_action_set(&header, test_action);
    cc_header_property_set(&header, test_prop_id);
    cc_header_node_cnt_set(&header, test_node_cnt);
    cc_header_parity_set(&header, true);
    uart_ctx.cnt_readable = 3;
    memcpy(uart_ctx.rx_buf, header.raw, 3);

    uart_ctx.cnt_writable = 1000;
    uart_ctx.rx_cnt       = 0;
    uart_ctx.tx_cnt       = 0;

    cc_node_tick(&node_ctx, 0);
    TEST_ASSERT_EQUAL(1, uart_ctx.tx_cnt);
    TEST_ASSERT_EQUAL(1, node_ctx.data_cnt);

    cc_node_tick(&node_ctx, 0);
    TEST_ASSERT_EQUAL(2, uart_ctx.tx_cnt);
    TEST_ASSERT_EQUAL(2, node_ctx.data_cnt);

    cc_node_tick(&node_ctx, 0);
    TEST_ASSERT_EQUAL(3, uart_ctx.tx_cnt);
    TEST_ASSERT_EQUAL(3, node_ctx.data_cnt);
    TEST_ASSERT_EQUAL(CC_NODE_STATE_RX_HEADER, node_ctx.state);
    TEST_ASSERT_EQUAL(CC_NODE_STATE_DEC_NODE_CNT, node_ctx.next_state);

    memcpy(header.raw, uart_ctx.tx_buf, 3); /* Compare the transmitted header. */
    TEST_ASSERT_EQUAL(test_action, cc_header_action_get(header));
    TEST_ASSERT_EQUAL(test_prop_id, cc_header_property_get(header));
    TEST_ASSERT_EQUAL(test_node_cnt + 1, cc_header_node_cnt_get(header));
    TEST_ASSERT_EQUAL(test_node_cnt + 1, node_ctx.node_cnt);
    TEST_ASSERT_TRUE(cc_header_parity_check(header));
}

//----------------------------------------------------------------------------------------------------------------------

void test_node_rx_header_state_sync_ack_ok(void)
{
    /* Test Params: */
    cc_action_t test_action      = CC_ACTION_SYNC;
    cc_sync_type_t test_sync     = CC_SYNC_ACK;
    cc_node_err_t expected_error = CC_SYNC_ERR_OTHER;

    /* Setup a node. */
    cc_node_ctx_t node_ctx = {0};
    node_dummy_config(&node_ctx);

    /* Setup the state. */
    node_ctx.state      = CC_NODE_STATE_RX_HEADER;
    node_ctx.next_state = CC_NODE_STATE_UNDEFINED;
    node_ctx.last_error = CC_NODE_ERR_INVALID_STATE;

    /* Write a dummy header. */
    cc_msg_header_t header = {0};
    cc_header_action_set(&header, test_action);
    cc_header_sync_type_set(&header, test_sync);
    uart_ctx.cnt_readable = 1;
    memcpy(uart_ctx.rx_buf, header.raw, 1);

    uart_ctx.cnt_writable = 1000;
    uart_ctx.rx_cnt       = 0;
    uart_ctx.tx_cnt       = 0;

    cc_node_tick(&node_ctx, 0);
    TEST_ASSERT_EQUAL(1, uart_ctx.tx_cnt);

    memcpy(header.raw, uart_ctx.tx_buf, 1); /* Compare the transmitted header. */
    TEST_ASSERT_EQUAL(test_action, cc_header_action_get(header));
    TEST_ASSERT_EQUAL(test_sync, cc_header_sync_type_get(header));
    TEST_ASSERT_EQUAL(expected_error, cc_header_sync_error_get(header));
    TEST_ASSERT_EQUAL(CC_NODE_STATE_RX_HEADER, node_ctx.state);
    TEST_ASSERT_EQUAL(CC_NODE_STATE_RX_HEADER, node_ctx.next_state);
}

//----------------------------------------------------------------------------------------------------------------------

void test_node_rx_header_state_sync_commit_ok(void)
{
    /* Test Params: */
    cc_action_t test_action      = CC_ACTION_SYNC;
    cc_sync_type_t test_sync     = CC_SYNC_COMMIT;
    cc_node_err_t expected_error = CC_SYNC_ERR_NONE; /* Error is not used in sync commit. */

    /* Setup a node. */
    cc_node_ctx_t node_ctx = {0};
    node_dummy_config(&node_ctx);

    /* Setup the state. */
    node_ctx.state      = CC_NODE_STATE_RX_HEADER;
    node_ctx.next_state = CC_NODE_STATE_UNDEFINED;
    node_ctx.last_error = CC_NODE_ERR_INVALID_STATE; /* Should not be used. */

    /* Write a dummy header. */
    cc_msg_header_t header = {0};
    cc_header_action_set(&header, test_action);
    cc_header_sync_type_set(&header, test_sync);
    uart_ctx.cnt_readable = 1;
    memcpy(uart_ctx.rx_buf, header.raw, 1);

    uart_ctx.cnt_writable = 1000;
    uart_ctx.rx_cnt       = 0;
    uart_ctx.tx_cnt       = 0;

    cc_node_tick(&node_ctx, 0);
    TEST_ASSERT_EQUAL(1, uart_ctx.tx_cnt);

    memcpy(header.raw, uart_ctx.tx_buf, 1); /* Compare the transmitted header. */
    TEST_ASSERT_EQUAL(test_action, cc_header_action_get(header));
    TEST_ASSERT_EQUAL(test_sync, cc_header_sync_type_get(header));
    TEST_ASSERT_EQUAL(expected_error, cc_header_sync_error_get(header));
    TEST_ASSERT_EQUAL(CC_NODE_STATE_RX_HEADER, node_ctx.state);
    TEST_ASSERT_EQUAL(CC_NODE_STATE_COMMIT_PROP, node_ctx.next_state);
}

//----------------------------------------------------------------------------------------------------------------------

void test_node_rx_header_state_read_action_parity_error(void)
{
    /* Test Params: */
    cc_action_t test_action   = CC_ACTION_READ;
    cc_prop_id_t test_prop_id = 1; /* Only one handler so must be one!*/
    uint16_t test_node_cnt    = 0;

    /* Setup a node. */
    cc_node_ctx_t node_ctx = {0};
    node_dummy_config(&node_ctx);

    /* Setup the state. */
    node_ctx.state      = CC_NODE_STATE_RX_HEADER;
    node_ctx.next_state = CC_NODE_STATE_UNDEFINED;

    /* Run the chain communication node. */
    cc_node_tick(&node_ctx, 0); /* Initial Tick. */

    /* Write a dummy header. */
    cc_msg_header_t header = {0};
    cc_header_action_set(&header, test_action);
    cc_header_property_set(&header, test_prop_id);
    cc_header_node_cnt_set(&header, test_node_cnt);
    cc_header_parity_set(&header, false); /* Intentionally set parity wrong. */
    uart_ctx.cnt_readable = 3;
    memcpy(uart_ctx.rx_buf, header.raw, 3);

    uart_ctx.cnt_writable = 1000;
    uart_ctx.rx_cnt       = 0;
    uart_ctx.tx_cnt       = 0;

    cc_node_tick(&node_ctx, 0);
    TEST_ASSERT_EQUAL(1, uart_ctx.tx_cnt);
    TEST_ASSERT_EQUAL(1, node_ctx.data_cnt);

    cc_node_tick(&node_ctx, 0);
    TEST_ASSERT_EQUAL(2, uart_ctx.tx_cnt);
    TEST_ASSERT_EQUAL(2, node_ctx.data_cnt);

    cc_node_tick(&node_ctx, 0);
    TEST_ASSERT_EQUAL(3, uart_ctx.tx_cnt);
    TEST_ASSERT_EQUAL(3, node_ctx.data_cnt);
    TEST_ASSERT_EQUAL(CC_NODE_STATE_RX_HEADER, node_ctx.state);
    TEST_ASSERT_EQUAL(CC_NODE_STATE_ERROR, node_ctx.next_state);
    TEST_ASSERT_EQUAL(CC_NODE_ERR_HEADER_PARITY, node_ctx.last_error);

    memcpy(header.raw, uart_ctx.tx_buf, 3); /* Compare the transmitted header. */
    TEST_ASSERT_EQUAL(test_action, cc_header_action_get(header));
    TEST_ASSERT_EQUAL(test_prop_id, cc_header_property_get(header));
    TEST_ASSERT_EQUAL(test_node_cnt + 1, cc_header_node_cnt_get(header));
    TEST_ASSERT_FALSE(cc_header_parity_check(header));
}

//----------------------------------------------------------------------------------------------------------------------

void test_node_rx_header_state_read_action_timeout_error(void)
{
    /* Test Params: */
    cc_action_t test_action   = CC_ACTION_READ;
    cc_prop_id_t test_prop_id = 1;    /* Only one handler so must be one!*/
    uint16_t test_node_cnt    = 0xFF; /* This value cause a carry from raw[1] to raw[2] */

    /* Setup a node. */
    cc_node_ctx_t node_ctx = {0};
    node_dummy_config(&node_ctx);

    /* Setup the state. */
    node_ctx.state      = CC_NODE_STATE_RX_HEADER;
    node_ctx.next_state = CC_NODE_STATE_UNDEFINED;

    /* Run the chain communication node. */
    cc_node_tick(&node_ctx, 0); /* Initial Tick. */

    /* Write a dummy header. */
    cc_msg_header_t header = {0};
    cc_header_action_set(&header, test_action);
    cc_header_property_set(&header, test_prop_id);
    cc_header_node_cnt_set(&header, test_node_cnt);
    cc_header_parity_set(&header, false); /* Intentionally set parity wrong. */
    uart_ctx.cnt_readable = 1;            /* Timeout will occur before full header is read. */
    memcpy(uart_ctx.rx_buf, header.raw, 3);

    uart_ctx.cnt_writable = 1000;
    uart_ctx.rx_cnt       = 0;
    uart_ctx.tx_cnt       = 0;

    cc_node_tick(&node_ctx, 0);
    TEST_ASSERT_EQUAL(1, uart_ctx.tx_cnt);
    TEST_ASSERT_EQUAL(1, node_ctx.data_cnt);

    cc_node_tick(&node_ctx, CC_NODE_TIMEOUT_MS + 1);
    TEST_ASSERT_EQUAL(1, uart_ctx.tx_cnt);
    TEST_ASSERT_EQUAL(1, node_ctx.data_cnt);
    TEST_ASSERT_EQUAL(CC_NODE_STATE_RX_HEADER, node_ctx.state);
    TEST_ASSERT_EQUAL(CC_NODE_STATE_ERROR, node_ctx.next_state);
    TEST_ASSERT_EQUAL(CC_NODE_ERR_TIMEOUT, node_ctx.last_error);
}

//----------------------------------------------------------------------------------------------------------------------

void test_node_rx_header_state_write_action_ok(void)
{
    /* Test Params: */
    cc_action_t test_action   = CC_ACTION_WRITE;
    cc_prop_id_t test_prop_id = 0;     /* Only one handler so must be zero!*/
    uint16_t test_node_cnt    = 0xFC0; /* This value cause a (negative) carry from raw[1] to raw[2] */

    /* Setup a node. */
    cc_node_ctx_t node_ctx = {0};
    node_dummy_config(&node_ctx);

    /* Setup the state. */
    node_ctx.state      = CC_NODE_STATE_RX_HEADER;
    node_ctx.next_state = CC_NODE_STATE_UNDEFINED;

    /* Run the chain communication node. */
    cc_node_tick(&node_ctx, 0); /* Initial Tick. */

    /* Write a dummy header. */
    cc_msg_header_t header = {0};
    cc_header_action_set(&header, test_action);
    cc_header_property_set(&header, test_prop_id);
    cc_header_node_cnt_set(&header, test_node_cnt);
    cc_header_parity_set(&header, true);
    uart_ctx.cnt_readable = 3;
    memcpy(uart_ctx.rx_buf, header.raw, 3);

    uart_ctx.cnt_writable = 1000;
    uart_ctx.rx_cnt       = 0;
    uart_ctx.tx_cnt       = 0;

    cc_node_tick(&node_ctx, 0);
    TEST_ASSERT_EQUAL(1, uart_ctx.tx_cnt);
    TEST_ASSERT_EQUAL(1, node_ctx.data_cnt);

    cc_node_tick(&node_ctx, 0);
    TEST_ASSERT_EQUAL(2, uart_ctx.tx_cnt);
    TEST_ASSERT_EQUAL(2, node_ctx.data_cnt);

    cc_node_tick(&node_ctx, 0);
    TEST_ASSERT_EQUAL(3, uart_ctx.tx_cnt);
    TEST_ASSERT_EQUAL(3, node_ctx.data_cnt);
    TEST_ASSERT_EQUAL(CC_NODE_STATE_RX_HEADER, node_ctx.state);
    TEST_ASSERT_EQUAL(CC_NODE_STATE_DEC_NODE_CNT, node_ctx.next_state);

    memcpy(header.raw, uart_ctx.tx_buf, 3); /* Compare the transmitted header. */
    TEST_ASSERT_EQUAL(test_action, cc_header_action_get(header));
    TEST_ASSERT_EQUAL(test_prop_id, cc_header_property_get(header));
    TEST_ASSERT_EQUAL(test_node_cnt - 1, cc_header_node_cnt_get(header));
    TEST_ASSERT_EQUAL(test_node_cnt, node_ctx.node_cnt);
    TEST_ASSERT_TRUE(cc_header_parity_check(header));
}

//----------------------------------------------------------------------------------------------------------------------

void test_node_rx_header_state_write_action_node_cnt_error(void)
{
    /* Test Params: */
    cc_action_t test_action   = CC_ACTION_WRITE;
    cc_prop_id_t test_prop_id = 1; /* Only one handler so must be one!*/
    uint16_t test_node_cnt    = 0; /* We can't write data to zero nodes! */

    /* Setup a node. */
    cc_node_ctx_t node_ctx = {0};
    node_dummy_config(&node_ctx);

    /* Setup the state. */
    node_ctx.state      = CC_NODE_STATE_RX_HEADER;
    node_ctx.next_state = CC_NODE_STATE_UNDEFINED;

    /* Run the chain communication node. */
    cc_node_tick(&node_ctx, 0); /* Initial Tick. */

    /* Write a dummy header. */
    cc_msg_header_t header = {0};
    cc_header_action_set(&header, test_action);
    cc_header_property_set(&header, test_prop_id);
    cc_header_node_cnt_set(&header, test_node_cnt);
    cc_header_parity_set(&header, true);
    uart_ctx.cnt_readable = 3;
    memcpy(uart_ctx.rx_buf, header.raw, 3);

    uart_ctx.cnt_writable = 1000;
    uart_ctx.rx_cnt       = 0;
    uart_ctx.tx_cnt       = 0;

    cc_node_tick(&node_ctx, 0);
    TEST_ASSERT_EQUAL(1, uart_ctx.tx_cnt);
    TEST_ASSERT_EQUAL(1, node_ctx.data_cnt);

    cc_node_tick(&node_ctx, 0);
    TEST_ASSERT_EQUAL(2, uart_ctx.tx_cnt);
    TEST_ASSERT_EQUAL(2, node_ctx.data_cnt);

    cc_node_tick(&node_ctx, 0);
    TEST_ASSERT_EQUAL(3, uart_ctx.tx_cnt);
    TEST_ASSERT_EQUAL(3, node_ctx.data_cnt);
    TEST_ASSERT_EQUAL(CC_NODE_STATE_RX_HEADER, node_ctx.state);
    TEST_ASSERT_EQUAL(CC_NODE_STATE_ERROR, node_ctx.next_state);
    TEST_ASSERT_EQUAL(CC_NODE_ERR_INVALID_STATE, node_ctx.last_error);

    memcpy(header.raw, uart_ctx.tx_buf, 3); /* Compare the transmitted header. */
    TEST_ASSERT_FALSE(cc_header_parity_check(header));
}

//----------------------------------------------------------------------------------------------------------------------

void test_node_dec_node_cnt_state_ok(void)
{
    /* Setup a node. */
    cc_node_ctx_t node_ctx = {0};
    node_dummy_config(&node_ctx);

    node_ctx.state        = CC_NODE_STATE_DEC_NODE_CNT;
    node_ctx.next_state   = CC_NODE_STATE_UNDEFINED;
    node_ctx.property_id  = 0; /* Only one handler so must be zero!*/
    node_ctx.staged_write = false;
    node_ctx.action       = CC_ACTION_READ;
    node_ctx.node_cnt     = 2;
    cc_node_tick(&node_ctx, 0);
    TEST_ASSERT_EQUAL(1, node_ctx.node_cnt);
    TEST_ASSERT_EQUAL(CC_NODE_STATE_DEC_NODE_CNT, node_ctx.state);
    TEST_ASSERT_EQUAL(CC_NODE_STATE_RX_PROP, node_ctx.next_state);

    /* Test Read flow where node_cnt = 1 */
    node_ctx.state        = CC_NODE_STATE_DEC_NODE_CNT;
    node_ctx.next_state   = CC_NODE_STATE_UNDEFINED;
    node_ctx.property_id  = 0; /* Only one handler so must be zero!*/
    node_ctx.staged_write = false;
    node_ctx.action       = CC_ACTION_READ;
    node_ctx.node_cnt     = 1;
    cc_node_tick(&node_ctx, 0);
    TEST_ASSERT_EQUAL(0, node_ctx.node_cnt);
    TEST_ASSERT_EQUAL(CC_NODE_STATE_DEC_NODE_CNT, node_ctx.state);
    TEST_ASSERT_EQUAL(CC_NODE_STATE_TX_PROP, node_ctx.next_state);

    /* Dont Test Read flow where node_cnt = 0, this can never happen ...*/

    /* Test Write flow where node_cnt > 1 */
    node_ctx.state        = CC_NODE_STATE_DEC_NODE_CNT;
    node_ctx.next_state   = CC_NODE_STATE_UNDEFINED;
    node_ctx.property_id  = 0; /* Only one handler so must be zero!*/
    node_ctx.staged_write = false;
    node_ctx.action       = CC_ACTION_WRITE;
    node_ctx.node_cnt     = 2;
    cc_node_tick(&node_ctx, 0);
    TEST_ASSERT_EQUAL(1, node_ctx.node_cnt);
    TEST_ASSERT_EQUAL(CC_NODE_STATE_DEC_NODE_CNT, node_ctx.state);
    TEST_ASSERT_EQUAL(CC_NODE_STATE_RX_PROP, node_ctx.next_state);

    /* Test Write flow where node_cnt = 1 */
    node_ctx.state        = CC_NODE_STATE_DEC_NODE_CNT;
    node_ctx.next_state   = CC_NODE_STATE_UNDEFINED;
    node_ctx.property_id  = 0; /* Only one handler so must be zero!*/
    node_ctx.staged_write = false;
    node_ctx.action       = CC_ACTION_WRITE;
    node_ctx.node_cnt     = 1;
    cc_node_tick(&node_ctx, 0);
    TEST_ASSERT_EQUAL(0, node_ctx.node_cnt);
    TEST_ASSERT_EQUAL(CC_NODE_STATE_DEC_NODE_CNT, node_ctx.state);
    TEST_ASSERT_EQUAL(CC_NODE_STATE_RX_PROP, node_ctx.next_state);

    /* Test Write flow where node_cnt = 0, don't stage write. */
    node_ctx.state        = CC_NODE_STATE_DEC_NODE_CNT;
    node_ctx.next_state   = CC_NODE_STATE_UNDEFINED;
    node_ctx.property_id  = 0; /* Only one handler so must be zero!*/
    node_ctx.staged_write = false;
    node_ctx.action       = CC_ACTION_WRITE;
    node_ctx.node_cnt     = 0;
    cc_node_tick(&node_ctx, 0);
    TEST_ASSERT_EQUAL(0, node_ctx.node_cnt);
    TEST_ASSERT_EQUAL(CC_NODE_STATE_DEC_NODE_CNT, node_ctx.state);
    TEST_ASSERT_EQUAL(CC_NODE_STATE_COMMIT_PROP, node_ctx.next_state);

    /* Test Write flow where node_cnt = 0, stage write. */
    node_ctx.state        = CC_NODE_STATE_DEC_NODE_CNT;
    node_ctx.next_state   = CC_NODE_STATE_UNDEFINED;
    node_ctx.property_id  = 0; /* Only one handler so must be zero!*/
    node_ctx.staged_write = true;
    node_ctx.action       = CC_ACTION_WRITE;
    node_ctx.node_cnt     = 0;
    cc_node_tick(&node_ctx, 0);
    TEST_ASSERT_EQUAL(0, node_ctx.node_cnt);
    TEST_ASSERT_EQUAL(CC_NODE_STATE_DEC_NODE_CNT, node_ctx.state);
    TEST_ASSERT_EQUAL(CC_NODE_STATE_RX_HEADER, node_ctx.next_state);
}

//----------------------------------------------------------------------------------------------------------------------

void test_node_tx_prop_state_ok(void)
{
    /* Setup a node. */
    cc_node_ctx_t node_ctx = {0};
    node_dummy_config(&node_ctx);

    node_ctx.state       = CC_NODE_STATE_TX_PROP;
    node_ctx.next_state  = CC_NODE_STATE_UNDEFINED;
    node_ctx.property_id = 0; /* Only one handler so must be zero!*/
    prop_ctx.success     = true;
    prop_ctx.size        = 10; /* Emulate a property of size 10 */
    memset(prop_ctx.data, 0xAA, prop_ctx.size);

    uart_ctx.rx_cnt = 0;
    uart_ctx.tx_cnt = 0;

    uart_ctx.cnt_writable = 5;
    cc_node_tick(&node_ctx, 0);
    TEST_ASSERT_EQUAL(5, uart_ctx.tx_cnt);
    TEST_ASSERT_EQUAL(5, node_ctx.data_cnt);
    uart_ctx.cnt_writable = 5;
    cc_node_tick(&node_ctx, 0);
    TEST_ASSERT_EQUAL(10, uart_ctx.tx_cnt);
    TEST_ASSERT_EQUAL(10, node_ctx.data_cnt);
    uart_ctx.cnt_writable = 5;
    cc_node_tick(&node_ctx, 0);
    TEST_ASSERT_EQUAL(13, uart_ctx.tx_cnt); /* Add 3 bytes COBS + CS overhead*/
    TEST_ASSERT_EQUAL(13, node_ctx.data_cnt);

    /* Verify the transmitted data. */
    uint8_t decoded_data[10] = {0};
    size_t decoded_size      = sizeof(decoded_data);
    cc_payload_cobs_decode(decoded_data, &decoded_size, uart_ctx.tx_buf, uart_ctx.tx_cnt);
    TEST_ASSERT_EQUAL(10, decoded_size);
    TEST_ASSERT_EQUAL_HEX8_ARRAY(prop_ctx.data, decoded_data, 10);

    TEST_ASSERT_EQUAL(CC_NODE_STATE_TX_PROP, node_ctx.state);
    TEST_ASSERT_EQUAL(CC_NODE_STATE_RX_HEADER, node_ctx.next_state);
}

//----------------------------------------------------------------------------------------------------------------------

void test_node_tx_prop_state_read_error_ok(void)
{
    /* Setup a node. */
    cc_node_ctx_t node_ctx = {0};
    node_dummy_config(&node_ctx);

    node_ctx.state       = CC_NODE_STATE_TX_PROP;
    node_ctx.next_state  = CC_NODE_STATE_UNDEFINED;
    node_ctx.property_id = 0;                             /* Only one handler so must be zero!*/
    cc_header_read_error_bit_set(&node_ctx.header, true); /* Set read_error bit. */
    node_ctx.last_error  = CC_NODE_ERR_CHECKSUM;          /* Set error for test.. */
    node_ctx.error_state = CC_NODE_STATE_TX_PROP;         /* Set error state for test.*/

    uart_ctx.cnt_writable = 1000;
    uart_ctx.rx_cnt       = 0;
    uart_ctx.tx_cnt       = 0;

    cc_node_tick(&node_ctx, 0);
    TEST_ASSERT_EQUAL(5, uart_ctx.tx_cnt);
    TEST_ASSERT_EQUAL(5, node_ctx.data_cnt);

    /* Verify the transmitted data. */
    uint8_t decoded_data[2] = {0};
    size_t decoded_size     = sizeof(decoded_data);
    cc_payload_cobs_decode(decoded_data, &decoded_size, uart_ctx.tx_buf, uart_ctx.tx_cnt);
    TEST_ASSERT_EQUAL(2, decoded_size);
    TEST_ASSERT_EQUAL(CC_NODE_ERR_CHECKSUM, decoded_data[0]);
    TEST_ASSERT_EQUAL(CC_NODE_STATE_TX_PROP, decoded_data[1]);

    TEST_ASSERT_EQUAL(CC_NODE_STATE_TX_PROP, node_ctx.state);
    TEST_ASSERT_EQUAL(CC_NODE_STATE_RX_HEADER, node_ctx.next_state);
}

//----------------------------------------------------------------------------------------------------------------------

void test_node_tx_prop_state_read_error_prop_id_nozero(void)
{
    /* Setup a node. */
    cc_node_ctx_t node_ctx = {0};
    node_dummy_config(&node_ctx);

    node_ctx.state      = CC_NODE_STATE_TX_PROP;
    node_ctx.next_state = CC_NODE_STATE_UNDEFINED;
    cc_header_read_error_bit_set(&node_ctx.header, true); /* Set read_error bit. */
    node_ctx.property_id = 5;                             /* Set property id > 0 */

    uart_ctx.cnt_writable = 1000;
    uart_ctx.rx_cnt       = 0;
    uart_ctx.tx_cnt       = 0;

    cc_node_tick(&node_ctx, 0);
    TEST_ASSERT_EQUAL(1, uart_ctx.tx_cnt);
    TEST_ASSERT_EQUAL(1, node_ctx.data_cnt);

    /* Verify the transmitted data. */
    TEST_ASSERT_EQUAL(0x00, uart_ctx.tx_buf[0]); /* Only one byte with value 0x00 should be sent. */

    TEST_ASSERT_EQUAL(CC_NODE_STATE_TX_PROP, node_ctx.state);
    TEST_ASSERT_EQUAL(CC_NODE_STATE_RX_HEADER, node_ctx.next_state);
    TEST_ASSERT_EQUAL(CC_NODE_ERR_INVALID_STATE, node_ctx.last_error); /* Error state */
}

//----------------------------------------------------------------------------------------------------------------------

void test_node_tx_prop_state_timeout_error(void)
{
    /* Setup a node. */
    cc_node_ctx_t node_ctx = {0};
    node_dummy_config(&node_ctx);

    node_ctx.state       = CC_NODE_STATE_TX_PROP;
    node_ctx.next_state  = CC_NODE_STATE_UNDEFINED;
    node_ctx.property_id = 0; /* Only one handler so must be zero!*/

    /* Run the chain communication node. */
    cc_node_tick(&node_ctx, 0); /* Initial Tick. */

    cc_node_tick(&node_ctx, CC_NODE_TIMEOUT_MS + 1);
    TEST_ASSERT_EQUAL(CC_NODE_STATE_TX_PROP, node_ctx.state);
    TEST_ASSERT_EQUAL(CC_NODE_STATE_ERROR, node_ctx.next_state);
    TEST_ASSERT_EQUAL(CC_NODE_ERR_TIMEOUT, node_ctx.last_error); /* Error state */
}

//----------------------------------------------------------------------------------------------------------------------

void test_node_tx_prop_state_error(void)
{
    /* Setup a node. */
    cc_node_ctx_t node_ctx = {0};
    node_dummy_config(&node_ctx);
    uart_ctx.cnt_writable = 1000;

    /* Case 1 : property not supported. */
    node_ctx.state       = CC_NODE_STATE_TX_PROP;
    node_ctx.next_state  = CC_NODE_STATE_TX_PROP;
    node_ctx.property_id = 5;    /* Set property id > 0 */
    uart_ctx.tx_buf[0]   = 0xFF; /* Fill TX buffer with dummy data to detect if anything is sent. */
    uart_ctx.tx_cnt      = 0;    /* Reset TX counter */
    cc_node_tick(&node_ctx, 0);
    TEST_ASSERT_EQUAL(0x00, uart_ctx.tx_buf[0]); /* Only one byte with value 0x00 should be sent. */
    TEST_ASSERT_EQUAL(CC_NODE_STATE_TX_PROP, node_ctx.state);
    TEST_ASSERT_EQUAL(CC_NODE_STATE_RX_HEADER, node_ctx.next_state);
    TEST_ASSERT_EQUAL(CC_NODE_ERR_PROP_NOT_SUPPORTED, node_ctx.last_error); /* Error state */

    /* Case 2 : No get handler. */
    cc_prop_handler_cb_t old_handler  = node_ctx.prop_list[0].handler.get;
    node_ctx.state                    = CC_NODE_STATE_TX_PROP;
    node_ctx.next_state               = CC_NODE_STATE_TX_PROP;
    node_ctx.property_id              = 0;
    uart_ctx.tx_buf[0]                = 0xFF; /* Fill TX buffer with dummy data to detect if anything is sent. */
    uart_ctx.tx_cnt                   = 0;    /* Reset TX counter */
    node_ctx.prop_list[0].handler.get = NULL; /* Remove get handler */
    cc_node_tick(&node_ctx, 0);
    TEST_ASSERT_EQUAL(0x00, uart_ctx.tx_buf[0]); /* Only one byte with value 0x00 should be sent. */
    TEST_ASSERT_EQUAL(CC_NODE_STATE_TX_PROP, node_ctx.state);
    TEST_ASSERT_EQUAL(CC_NODE_STATE_RX_HEADER, node_ctx.next_state);
    TEST_ASSERT_EQUAL(CC_NODE_ERR_READ_NOT_SUPPORTED, node_ctx.last_error); /* Error state */
    node_ctx.prop_list[0].handler.get = old_handler;                        /* Restore get handler */

    /* Case 3 : Handler returns false. */
    node_ctx.state       = CC_NODE_STATE_TX_PROP;
    node_ctx.next_state  = CC_NODE_STATE_TX_PROP;
    node_ctx.property_id = 0;
    uart_ctx.tx_buf[0]   = 0xFF;  /* Fill TX buffer with dummy data to detect if anything is sent. */
    uart_ctx.tx_cnt      = 0;     /* Reset TX counter */
    prop_ctx.success     = false; /* Set handler to return false. */
    cc_node_tick(&node_ctx, 0);
    TEST_ASSERT_EQUAL(0x00, uart_ctx.tx_buf[0]); /* Only one byte with value 0x00 should be sent. */
    TEST_ASSERT_EQUAL(CC_NODE_STATE_TX_PROP, node_ctx.state);
    TEST_ASSERT_EQUAL(CC_NODE_STATE_RX_HEADER, node_ctx.next_state);
    TEST_ASSERT_EQUAL(CC_NODE_ERR_READ_CB, node_ctx.last_error); /* Error state */
    node_ctx.prop_list[0].handler.get = old_handler;             /* Restore get handler */

    /* Case 4 : COBS encoding fails. */
    node_ctx.state       = CC_NODE_STATE_TX_PROP;
    node_ctx.next_state  = CC_NODE_STATE_TX_PROP;
    node_ctx.property_id = 0;
    uart_ctx.tx_buf[0]   = 0xFF;                     /* Fill TX buffer with dummy data to detect if anything is sent. */
    uart_ctx.tx_cnt      = 0;                        /* Reset TX counter */
    prop_ctx.size        = CC_PROPERTY_SIZE_MAX + 1; /* Set property size larger than max payload size. */
    prop_ctx.success     = true;
    cc_node_tick(&node_ctx, 0);
    TEST_ASSERT_EQUAL(0x00, uart_ctx.tx_buf[0]); /* Only one byte with value 0x00 should be sent. */
    TEST_ASSERT_EQUAL(CC_NODE_STATE_TX_PROP, node_ctx.state);
    TEST_ASSERT_EQUAL(CC_NODE_STATE_RX_HEADER, node_ctx.next_state);
    TEST_ASSERT_EQUAL(CC_NODE_ERR_COBS_ENC, node_ctx.last_error); /* Error state */
}

//------------------------------------------------------------------------------------------------------------------------------------------------------

void test_node_rx_prop_state_no_forwarding_ok(void)
{
    /* Setup a node. */
    cc_node_ctx_t node_ctx = {0};
    node_dummy_config(&node_ctx);
    node_ctx.state       = CC_NODE_STATE_RX_PROP;
    node_ctx.next_state  = CC_NODE_STATE_UNDEFINED;
    node_ctx.property_id = 0;               /* Only one handler so must be zero!*/
    node_ctx.action      = CC_ACTION_WRITE; /* No forwarding */
    node_ctx.node_cnt    = 0;               /* No forwarding */

    uint8_t test_data[11] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99};
    /* Write a valid cobs payload in the rx buffer. */
    test_data[sizeof(test_data) - 1] = cc_checksum_calculate(test_data, sizeof(test_data) - 1);
    uart_ctx.cnt_readable            = 1000;
    cc_payload_cobs_encode(uart_ctx.rx_buf, &uart_ctx.cnt_readable, test_data, sizeof(test_data));
    uart_ctx.cnt_writable = 1000;

    cc_node_tick(&node_ctx, 0);

    TEST_ASSERT_EQUAL(0, uart_ctx.tx_cnt); /* Check that no bytes were sent. */
    TEST_ASSERT_EQUAL(CC_NODE_STATE_RX_PROP, node_ctx.state);
    TEST_ASSERT_EQUAL(CC_NODE_STATE_DEC_NODE_CNT, node_ctx.next_state);
}

//------------------------------------------------------------------------------------------------------------------------------------------------------

void test_node_rx_prop_state_forwarding_ok(void)
{
    /* Setup a node. */
    cc_node_ctx_t node_ctx = {0};
    node_dummy_config(&node_ctx);
    node_ctx.property_id = 0; /* Only one handler so must be zero!*/

    uint8_t test_data[11] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99};
    /* Write a valid cobs payload in the rx buffer. */
    test_data[sizeof(test_data) - 1] = cc_checksum_calculate(test_data, sizeof(test_data) - 1);
    uart_ctx.cnt_readable            = 1000;
    cc_payload_cobs_encode(uart_ctx.rx_buf, &uart_ctx.cnt_readable, test_data, sizeof(test_data));
    uart_ctx.cnt_readable = 1000;

    /* Case 1 : Forwarding due to broadcast action. */
    node_ctx.next_state = CC_NODE_STATE_RX_PROP;
    node_ctx.action     = CC_ACTION_BROADCAST; /* Forwarding */
    node_ctx.node_cnt   = 0;                   /* No forwarding */
    uart_ctx.rx_cnt     = 0;
    uart_ctx.tx_cnt     = 0;

    uart_ctx.cnt_writable = 7;
    cc_node_tick(&node_ctx, 0); /* Write 7 bytes in first go. */
    TEST_ASSERT_EQUAL(7, uart_ctx.tx_cnt);
    uart_ctx.cnt_writable = 7;
    cc_node_tick(&node_ctx, 0); /* Write 6 more bytes in second go. */
    TEST_ASSERT_EQUAL(7 + 6, uart_ctx.tx_cnt);

    TEST_ASSERT_EQUAL_HEX8_ARRAY(uart_ctx.rx_buf, uart_ctx.tx_buf,
                                 uart_ctx.tx_cnt); /* Check that the same bytes were sent. */
    TEST_ASSERT_EQUAL(CC_NODE_STATE_RX_PROP, node_ctx.state);
    TEST_ASSERT_EQUAL(CC_NODE_STATE_DEC_NODE_CNT, node_ctx.next_state);

    /* Case 2 : Forwarding due to Node cnt. */
    node_ctx.next_state = CC_NODE_STATE_RX_PROP;
    node_ctx.action     = CC_ACTION_WRITE; /* No Forwarding */
    node_ctx.node_cnt   = 1;               /* Forwarding */
    uart_ctx.rx_cnt     = 0;
    uart_ctx.tx_cnt     = 0;

    uart_ctx.cnt_writable = 7;
    cc_node_tick(&node_ctx, 0); /* Write 7 bytes in first go. */
    TEST_ASSERT_EQUAL(7, uart_ctx.tx_cnt);
    uart_ctx.cnt_writable = 7;
    cc_node_tick(&node_ctx, 0); /* Write 6 more bytes in second go. */
    TEST_ASSERT_EQUAL(7 + 6, uart_ctx.tx_cnt);

    TEST_ASSERT_EQUAL_HEX8_ARRAY(uart_ctx.rx_buf, uart_ctx.tx_buf,
                                 uart_ctx.tx_cnt); /* Check that the same bytes were sent. */
    TEST_ASSERT_EQUAL(CC_NODE_STATE_RX_PROP, node_ctx.state);
    TEST_ASSERT_EQUAL(CC_NODE_STATE_DEC_NODE_CNT, node_ctx.next_state);
}

//------------------------------------------------------------------------------------------------------------------------------------------------------

void test_node_rx_prop_state_error(void)
{
    /* Setup a node. */
    cc_node_ctx_t node_ctx = {0};
    node_dummy_config(&node_ctx);
    node_ctx.property_id = 0;                   /* Only one handler so must be zero!*/
    node_ctx.action      = CC_ACTION_BROADCAST; /* Forwarding */
    node_ctx.node_cnt    = 0;                   /* No forwarding */

    uint8_t test_data[11] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99};
    /* Write a valid cobs payload in the rx buffer. */
    test_data[sizeof(test_data) - 1] = cc_checksum_calculate(test_data, sizeof(test_data) - 1);
    uart_ctx.cnt_readable            = 1000;
    cc_payload_cobs_encode(uart_ctx.rx_buf, &uart_ctx.cnt_readable, test_data, sizeof(test_data));
    uart_ctx.cnt_readable   = 1000;
    uart_ctx.cnt_writable   = 1000;
    uint8_t original_byte_7 = uart_ctx.rx_buf[7];

    /* Case 1 : COBS decoding fail. */
    uart_ctx.rx_buf[7]  = 0x00; /* Intentionally corrupt the COBS data. */
    node_ctx.next_state = CC_NODE_STATE_RX_PROP;
    uart_ctx.rx_cnt     = 0;
    uart_ctx.tx_cnt     = 0;

    cc_node_tick(&node_ctx, 0);
    TEST_ASSERT_EQUAL(8, uart_ctx.tx_cnt); /* Only 8 bytes were sent before the error. */
    TEST_ASSERT_EQUAL(CC_NODE_STATE_RX_PROP, node_ctx.state);
    TEST_ASSERT_EQUAL(CC_NODE_STATE_ERROR, node_ctx.next_state);
    TEST_ASSERT_EQUAL(CC_NODE_ERR_COBS_DEC, node_ctx.last_error); /* Error state */
    uart_ctx.rx_buf[7] = original_byte_7;                         /* Restore the original byte. */

    /* Case 2 : Checksum fail. */
    uart_ctx.rx_buf[11]++; /* Intentionally corrupt the checksum byte. */
    node_ctx.next_state = CC_NODE_STATE_RX_PROP;
    uart_ctx.rx_cnt     = 0;
    uart_ctx.tx_cnt     = 0;

    cc_node_tick(&node_ctx, 0);
    TEST_ASSERT_EQUAL(13, uart_ctx.tx_cnt); /* Whole payload was sent before the error. */
    TEST_ASSERT_EQUAL(CC_NODE_STATE_RX_PROP, node_ctx.state);
    TEST_ASSERT_EQUAL(CC_NODE_STATE_ERROR, node_ctx.next_state);
    TEST_ASSERT_EQUAL(CC_NODE_ERR_CHECKSUM, node_ctx.last_error); /* Error state */
    uart_ctx.rx_buf[11]--;                                        /* Restore the original checksum byte. */
}

//----------------------------------------------------------------------------------------------------------------------

void test_node_rx_prop_state_timeout_error(void)
{
    /* Setup a node. */
    cc_node_ctx_t node_ctx = {0};
    node_dummy_config(&node_ctx);

    node_ctx.state       = CC_NODE_STATE_RX_PROP;
    node_ctx.next_state  = CC_NODE_STATE_UNDEFINED;
    node_ctx.property_id = 0; /* Only one handler so must be zero!*/

    /* Run the chain communication node. */
    cc_node_tick(&node_ctx, 0); /* Initial Tick. */

    cc_node_tick(&node_ctx, CC_NODE_TIMEOUT_MS + 1);
    TEST_ASSERT_EQUAL(CC_NODE_STATE_RX_PROP, node_ctx.state);
    TEST_ASSERT_EQUAL(CC_NODE_STATE_ERROR, node_ctx.next_state);
    TEST_ASSERT_EQUAL(CC_NODE_ERR_TIMEOUT, node_ctx.last_error); /* Error state */
}

//----------------------------------------------------------------------------------------------------------------------

void test_node_commit_prop_state_ok(void)
{
    /* Setup a node. */
    cc_node_ctx_t node_ctx = {0};
    node_dummy_config(&node_ctx);
    node_ctx.next_state    = CC_NODE_STATE_COMMIT_PROP;
    node_ctx.property_size = 10;
    randomize_array(prop_ctx.data, node_ctx.property_size);

    node_ctx.property_id = 0; /* Only one handler so must be zero!*/
    prop_ctx.success     = true;

    /* Run the chain communication node. */
    uart_ctx.tx_busy = true; /* TX still busy -> do nothing. */
    cc_node_tick(&node_ctx, 0);
    TEST_ASSERT_EQUAL(CC_NODE_STATE_COMMIT_PROP, node_ctx.state);
    TEST_ASSERT_EQUAL(CC_NODE_STATE_UNDEFINED, node_ctx.next_state);

    uart_ctx.tx_busy = false; /* TX still done -> call handler. */
    cc_node_tick(&node_ctx, 0);
    TEST_ASSERT_EQUAL(node_ctx.property_size, prop_ctx.size);
    TEST_ASSERT_EQUAL_HEX8_ARRAY(prop_ctx.data, prop_ctx.data, node_ctx.property_size);
    TEST_ASSERT_EQUAL(CC_NODE_STATE_COMMIT_PROP, node_ctx.state);
    TEST_ASSERT_EQUAL(CC_NODE_STATE_RX_HEADER, node_ctx.next_state);
}

//----------------------------------------------------------------------------------------------------------------------

void test_node_commit_prop_state_fail(void)
{
    /* Setup a node. */
    cc_node_ctx_t node_ctx = {0};
    node_dummy_config(&node_ctx);
    node_ctx.property_size = 10;
    randomize_array(prop_ctx.data, node_ctx.property_size);
    uart_ctx.tx_busy = false; /* Ensure that TX is not busy. */

    /* Case 1 : invalid property ID */
    node_ctx.next_state  = CC_NODE_STATE_COMMIT_PROP;
    node_ctx.property_id = 5; /* Invalid */
    prop_ctx.success     = true;

    cc_node_tick(&node_ctx, 0);
    TEST_ASSERT_EQUAL(CC_NODE_STATE_COMMIT_PROP, node_ctx.state);
    TEST_ASSERT_EQUAL(CC_NODE_STATE_ERROR, node_ctx.next_state);
    TEST_ASSERT_EQUAL(CC_NODE_ERR_PROP_NOT_SUPPORTED, node_ctx.last_error); /* Error state */

    /* Case 2 : no set handler. */
    node_ctx.next_state                       = CC_NODE_STATE_COMMIT_PROP;
    node_ctx.property_id                      = 0; /* Only one handler so must be zero!*/
    prop_ctx.success                          = true;
    cc_prop_handler_cb_t original_set_handler = node_ctx.prop_list[0].handler.set;
    node_ctx.prop_list[0].handler.set         = NULL; /* Remove set handler */

    cc_node_tick(&node_ctx, 0);
    TEST_ASSERT_EQUAL(CC_NODE_STATE_COMMIT_PROP, node_ctx.state);
    TEST_ASSERT_EQUAL(CC_NODE_STATE_ERROR, node_ctx.next_state);
    TEST_ASSERT_EQUAL(CC_NODE_ERR_WRITE_NOT_SUPPORTED, node_ctx.last_error); /* Error state */
    node_ctx.prop_list[0].handler.set = original_set_handler;                /* Restore set handler */

    /* Case 3 : invalid property ID */
    node_ctx.next_state  = CC_NODE_STATE_COMMIT_PROP;
    node_ctx.property_id = 0;     /* Only one handler so must be zero!*/
    prop_ctx.success     = false; /* Simulate handler failure. */

    cc_node_tick(&node_ctx, 0);
    TEST_ASSERT_EQUAL(CC_NODE_STATE_COMMIT_PROP, node_ctx.state);
    TEST_ASSERT_EQUAL(CC_NODE_STATE_ERROR, node_ctx.next_state);
    TEST_ASSERT_EQUAL(CC_NODE_ERR_WRITE_CB, node_ctx.last_error); /* Error state */
}

//----------------------------------------------------------------------------------------------------------------------

void test_node_error_state_ok(void)
{
    /* Setup a node. */
    cc_node_ctx_t node_ctx = {0};
    node_dummy_config(&node_ctx);

    uart_ctx.tx_busy      = true;
    uart_ctx.cnt_writable = 1000;

    /* No Data */
    node_ctx.next_state = CC_NODE_STATE_ERROR;
    cc_node_tick(&node_ctx, 0);
    TEST_ASSERT_EQUAL(CC_NODE_STATE_ERROR, node_ctx.state);
    TEST_ASSERT_EQUAL(CC_NODE_STATE_UNDEFINED, node_ctx.next_state);

    /* Handle incoming data. */
    uart_ctx.cnt_readable = 5;
    randomize_array(uart_ctx.tx_buf, uart_ctx.tx_cnt);
    cc_node_tick(&node_ctx, 0);
    TEST_ASSERT_EQUAL(5, uart_ctx.tx_cnt);
    TEST_ASSERT_EQUAL_HEX8_ARRAY(((uint8_t[5]) {0, 0, 0, 0, 0}), uart_ctx.tx_buf, 5);
    TEST_ASSERT_EQUAL(CC_NODE_STATE_ERROR, node_ctx.state);
    TEST_ASSERT_EQUAL(CC_NODE_STATE_UNDEFINED, node_ctx.next_state);

    /* Restart Timer */
    uart_ctx.tx_busy = true;
    cc_node_tick(&node_ctx, CC_NODE_TIMEOUT_MS + 1);
    TEST_ASSERT_EQUAL(CC_NODE_STATE_ERROR, node_ctx.state);
    TEST_ASSERT_EQUAL(CC_NODE_STATE_UNDEFINED, node_ctx.next_state);

    /* Timer Elapses */
    uart_ctx.tx_busy = false;
    cc_node_tick(&node_ctx, 3 * (CC_NODE_TIMEOUT_MS + 1));
    TEST_ASSERT_EQUAL(CC_NODE_STATE_ERROR, node_ctx.state);
    TEST_ASSERT_EQUAL(CC_NODE_STATE_RX_HEADER, node_ctx.next_state);
}

//----------------------------------------------------------------------------------------------------------------------

void test_cc_node_is_busy_ok(void)
{
    /* Setup a node. */
    cc_node_ctx_t node_ctx = {0};
    node_dummy_config(&node_ctx);

    /* Not Busy */
    node_ctx.state   = CC_NODE_STATE_RX_HEADER;
    uart_ctx.tx_busy = false;
    TEST_ASSERT_FALSE(cc_node_is_busy(&node_ctx));

    /* Busy */
    node_ctx.state   = CC_NODE_STATE_RX_HEADER;
    uart_ctx.tx_busy = true;
    TEST_ASSERT_TRUE(cc_node_is_busy(&node_ctx));

    /* Busy */
    node_ctx.state   = CC_NODE_STATE_RX_PROP;
    uart_ctx.tx_busy = false;
    TEST_ASSERT_TRUE(cc_node_is_busy(&node_ctx));
}

//----------------------------------------------------------------------------------------------------------------------

void test_cc_node_tick_invalid_state(void)
{
    /* Setup a node. */
    cc_node_ctx_t node_ctx = {0};
    node_dummy_config(&node_ctx);

    node_ctx.state      = CC_NODE_STATE_UNDEFINED;
    node_ctx.next_state = CC_NODE_STATE_UNDEFINED;

    cc_node_tick(&node_ctx, 0);
    TEST_ASSERT_EQUAL(CC_NODE_STATE_UNDEFINED, node_ctx.state);
    TEST_ASSERT_EQUAL(CC_NODE_STATE_UNDEFINED, node_ctx.next_state);
}

//======================================================================================================================
//                                                   MAIN TEST RUNNER
//======================================================================================================================

int main(void)
{
    UNITY_BEGIN();

    /* Test shared functions. */
    RUN_TEST(test_cc_cobs_encode_decode_ok);
    RUN_TEST(test_cc_cobs_encode_nok);
    RUN_TEST(test_cc_cobs_decode_nok);
    RUN_TEST(test_cc_checksum_update);
    RUN_TEST(test_cc_checksum_calculate);
    RUN_TEST(test_cc_parity_check);
    RUN_TEST(test_cc_node_cnt_set_get);
    RUN_TEST(test_cc_header_action_set_get);
    RUN_TEST(test_cc_header_staging_bit_set_get);
    RUN_TEST(test_cc_header_property_set_get);
    RUN_TEST(test_cc_header_parity_set_get);
    RUN_TEST(test_cc_header_sync_type_set_get);
    RUN_TEST(test_cc_header_sync_error_set_get);

    /* Test Master functions. */
    // RUN_TEST(test_cc_master_prop_read);
    RUN_TEST(test_master_read_nodes);
    RUN_TEST(test_master_write_nodes);

    /* Test Node functions. */
    RUN_TEST(test_node_init);
    RUN_TEST(test_node_rx_header_state_read_action_ok);
    RUN_TEST(test_node_rx_header_state_sync_ack_ok);
    RUN_TEST(test_node_rx_header_state_sync_commit_ok);
    RUN_TEST(test_node_rx_header_state_read_action_parity_error);
    RUN_TEST(test_node_rx_header_state_read_action_timeout_error);
    RUN_TEST(test_node_rx_header_state_write_action_ok);
    RUN_TEST(test_node_rx_header_state_write_action_node_cnt_error);
    RUN_TEST(test_node_dec_node_cnt_state_ok);
    RUN_TEST(test_node_tx_prop_state_ok);
    RUN_TEST(test_node_tx_prop_state_read_error_ok);
    RUN_TEST(test_node_tx_prop_state_read_error_prop_id_nozero);
    RUN_TEST(test_node_tx_prop_state_timeout_error);
    RUN_TEST(test_node_tx_prop_state_error);
    RUN_TEST(test_node_rx_prop_state_no_forwarding_ok);
    RUN_TEST(test_node_rx_prop_state_forwarding_ok);
    RUN_TEST(test_node_rx_prop_state_timeout_error);
    RUN_TEST(test_node_rx_prop_state_error);
    RUN_TEST(test_node_commit_prop_state_ok);
    RUN_TEST(test_node_commit_prop_state_fail);
    RUN_TEST(test_node_error_state_ok);
    RUN_TEST(test_cc_node_is_busy_ok);
    RUN_TEST(test_cc_node_tick_invalid_state);

    return UNITY_END();
}

//======================================================================================================================
//                                                         PRIVATE FUNCTIONS
//======================================================================================================================

void randomize_array(uint8_t *array, size_t size)
{
    for (size_t i = 0; i < size; i++) {
        array[i] = (uint8_t)(rand() % 0xFF);
    }
}

//----------------------------------------------------------------------------------------------------------------------

size_t uart_dummy_read(void *uart_userdata, uint8_t *data, size_t size)
{
    uart_dummy_ctx_t *ctx = (uart_dummy_ctx_t *)uart_userdata;

    size_t s = ctx->cnt_readable < size ? ctx->cnt_readable : size;
    memcpy(data, ctx->rx_buf + ctx->rx_cnt, s);

    // for (size_t i = 0; i < s; i++) {
    //     printf("RX %02X\n", data[i]);
    // }

    ctx->rx_cnt += s;
    ctx->cnt_readable -= s;
    return s;
}

//----------------------------------------------------------------------------------------------------------------------

size_t uart_dummy_cnt_readable(void *uart_userdata)
{
    uart_dummy_ctx_t *ctx = (uart_dummy_ctx_t *)uart_userdata;

    return ctx->cnt_readable;
}

//----------------------------------------------------------------------------------------------------------------------

size_t uart_dummy_write(void *uart_userdata, const uint8_t *data, size_t size)
{
    uart_dummy_ctx_t *ctx = (uart_dummy_ctx_t *)uart_userdata;

    if (size > ctx->cnt_writable) {
        size = ctx->cnt_writable;
    }

    memcpy(ctx->tx_buf + ctx->tx_cnt, data, size);

    // for (size_t i = 0; i < size; i++) {
    //     printf("TX %02X\n", data[i]);
    // }

    ctx->cnt_writable -= size;
    ctx->tx_busy = true;
    ctx->tx_cnt += size;

    return size;
}

//----------------------------------------------------------------------------------------------------------------------

size_t uart_dummy_cnt_writable(void *uart_userdata)
{
    uart_dummy_ctx_t *ctx = (uart_dummy_ctx_t *)uart_userdata;

    return ctx->cnt_writable;
}

//----------------------------------------------------------------------------------------------------------------------

bool uart_dummy_tx_buff_empty(void *uart_userdata)
{
    uart_dummy_ctx_t *ctx = (uart_dummy_ctx_t *)uart_userdata;

    return ctx->tx_busy == false;
}

//----------------------------------------------------------------------------------------------------------------------

bool uart_dummy_is_busy(void *uart_userdata)
{
    uart_dummy_ctx_t *ctx = (uart_dummy_ctx_t *)uart_userdata;

    return ctx->tx_busy;
}

//----------------------------------------------------------------------------------------------------------------------

void uart_dummy_flush_rx_buff(void *uart_userdata)
{
    uart_dummy_ctx_t *ctx = (uart_dummy_ctx_t *)uart_userdata;
    ctx->rx_cnt           = 0;
    ctx->cnt_readable     = 0;
}

//----------------------------------------------------------------------------------------------------------------------

bool prop_dummy_set_handler(void *userdata, uint16_t node_idx, uint8_t *buf, size_t *size)
{
    (void)node_idx;
    prop_dummy_ctx_t *ctx = (prop_dummy_ctx_t *)userdata;
    ctx->size             = *size;
    memcpy(ctx->data, buf, *size);
    return ctx->success;
}

//----------------------------------------------------------------------------------------------------------------------

bool prop_dummy_get_handler(void *userdata, uint16_t node_idx, uint8_t *buf, size_t *size)
{
    (void)node_idx;
    prop_dummy_ctx_t *ctx = (prop_dummy_ctx_t *)userdata;
    *size                 = ctx->size;
    memcpy(buf, ctx->data, *size);
    return ctx->success;
}

//----------------------------------------------------------------------------------------------------------------------

void node_dummy_config(cc_node_ctx_t *node_ctx)
{
    memset(&uart_ctx, 0, sizeof(uart_dummy_ctx_t));
    memset(&prop_ctx, 0, sizeof(prop_dummy_ctx_t));
    memset(node_ctx, 0, sizeof(cc_node_ctx_t));
    node_ctx->prop_list          = prop_list;
    node_ctx->prop_list_size     = sizeof(prop_list) / sizeof(prop_list[0]);
    node_ctx->prop_userdata      = &prop_ctx;
    node_ctx->uart.read          = uart_dummy_read;
    node_ctx->uart.cnt_readable  = uart_dummy_cnt_readable;
    node_ctx->uart.write         = uart_dummy_write;
    node_ctx->uart.cnt_writable  = uart_dummy_cnt_writable;
    node_ctx->uart.tx_buff_empty = uart_dummy_tx_buff_empty;
    node_ctx->uart.is_busy       = uart_dummy_is_busy;
    node_ctx->uart_userdata      = &uart_ctx;
}

//----------------------------------------------------------------------------------------------------------------------