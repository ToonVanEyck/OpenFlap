#include "chain_comm_master.h"
#include "chain_comm_master_test.h"
#include "chain_comm_node.h"
#include "chain_comm_node_test.h"
#include "test_properties.h"

#include "unity.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// static cc_test_master_ctx_t test_master_ctx;
// static cc_test_node_group_ctx_t node_test_grp;

void randomize_array(uint8_t *array, size_t size)
{
    for (size_t i = 0; i < size; i++) {
        array[i] = (uint8_t)(rand() % 0xFF);
    }
}

#define TEST_ASSERT_COBS_ENCODED(_encoded_data, _encoded_data_len)                                                     \
    do {                                                                                                               \
        for (size_t i = 0; i < _encoded_data_len - 1; i++) {                                                           \
            TEST_ASSERT_NOT_EQUAL_MESSAGE(0, _encoded_data[i],                                                         \
                                          "COBS encoding failed, zero byte found in encoded data");                    \
        }                                                                                                              \
        TEST_ASSERT_EQUAL_MESSAGE(0, _encoded_data[_encoded_data_len - 1],                                             \
                                  "COBS encoding failed, last byte is not zero");                                      \
    } while (0);

// void test_read_write_property(uint8_t node_cnt, cc_prop_id_t property)
// {
//     TEST_ASSERT_TRUE(property == PROP_STATIC_RW || property == PROP_DYNAMIC_RW);
//     TEST_ASSERT_TRUE(255 > node_cnt && node_cnt > 0);

//     cc_master_err_t err = CC_MASTER_OK;

//     /* Initialize master. */
//     setup_cc_master_property_list_handlers();
//     cc_test_master_init(&test_master_ctx);

//     TEST_ASSERT_NULL(test_master_ctx.node_data);
//     TEST_ASSERT_EQUAL(0, test_master_ctx.node_cnt);

//     /* Initialize the nodes. */
//     setup_cc_node_property_list_handlers();
//     cc_test_node_init(&node_test_grp, node_cnt, &test_master_ctx);

//     usleep(100000); // Wait a bit for threads to start

//     /* Try a read all command */
//     for (int i = 0; i < node_cnt; i++) {
//         randomize_array(node_test_grp.node_list[i].node_data, TEST_PROP_SIZE);
//     }

//     err = cc_master_prop_read_all(&test_master_ctx.master_ctx, property);
//     usleep(10000);
//     TEST_ASSERT_EQUAL(CC_MASTER_OK, err);

//     TEST_ASSERT_NOT_NULL(test_master_ctx.node_data);
//     TEST_ASSERT_EQUAL(node_cnt, test_master_ctx.node_cnt);

//     for (int i = 0; i < node_cnt; i++) {
//         TEST_ASSERT_EQUAL_HEX8_ARRAY(test_master_ctx.node_data + (i * TEST_PROP_SIZE),
//                                      node_test_grp.node_list[i].node_data, TEST_PROP_SIZE);
//         TEST_ASSERT_EQUAL(rxHeader, node_test_grp.node_list[i].node_ctx.state);
//     }

//     /* Try a write all command */
//     randomize_array(test_master_ctx.node_data, TEST_PROP_SIZE); /* Write all uses data from node 0 */

//     err = cc_master_prop_write_all(&test_master_ctx.master_ctx, property);
//     usleep(10000);
//     TEST_ASSERT_EQUAL(CC_MASTER_OK, err);

//     for (int i = 0; i < node_cnt; i++) {
//         TEST_ASSERT_EQUAL_HEX8_ARRAY(test_master_ctx.node_data, node_test_grp.node_list[i].node_data,
//         TEST_PROP_SIZE); TEST_ASSERT_EQUAL(rxHeader, node_test_grp.node_list[i].node_ctx.state);
//     }

//     /* Try a write sequential command */
//     memset(test_master_ctx.node_data, 0xff, TEST_PROP_SIZE * node_cnt);

//     for (int i = 0; i < node_cnt; i++) {
//         memset(node_test_grp.node_list[i].node_data, 0x00, TEST_PROP_SIZE);
//     }

//     err = cc_master_prop_write_seq(&test_master_ctx.master_ctx, property);
//     usleep(10000);
//     TEST_ASSERT_EQUAL(CC_MASTER_OK, err);

//     for (int i = 0; i < node_cnt; i++) {
//         if (i % 2 == 0) {
//             TEST_ASSERT_EQUAL_HEX8_ARRAY(test_master_ctx.node_data + (i * TEST_PROP_SIZE),
//                                          node_test_grp.node_list[i].node_data, TEST_PROP_SIZE);
//         } else {
//             TEST_ASSERT_EQUAL_HEX8_ARRAY((uint8_t[TEST_PROP_SIZE]) {0x00}, node_test_grp.node_list[i].node_data,
//                                          TEST_PROP_SIZE);
//         }
//         TEST_ASSERT_EQUAL(rxHeader, node_test_grp.node_list[i].node_ctx.state);
//     }
// }

// //----------------------------------------------------------------------------------------------------------------------

// void test_read_only_property(uint8_t node_cnt, cc_prop_id_t property)
// {
//     TEST_ASSERT_TRUE(property == PROP_STATIC_RO || property == PROP_DYNAMIC_RO);
//     TEST_ASSERT_TRUE(255 > node_cnt && node_cnt > 0);

//     cc_master_err_t err = CC_MASTER_OK;

//     /* Initialize master. */
//     setup_cc_master_property_list_handlers();
//     cc_test_master_init(&test_master_ctx);

//     TEST_ASSERT_NULL(test_master_ctx.node_data);
//     TEST_ASSERT_EQUAL(0, test_master_ctx.node_cnt);

//     /* Initialize the nodes. */
//     setup_cc_node_property_list_handlers();
//     cc_test_node_init(&node_test_grp, node_cnt, &test_master_ctx);

//     usleep(100000); // Wait a bit for threads to start
//     /* Try a read all command */
//     for (int i = 0; i < node_cnt; i++) {
//         randomize_array(node_test_grp.node_list[i].node_data, TEST_PROP_SIZE);
//     }

//     err = cc_master_prop_read_all(&test_master_ctx.master_ctx, property);
//     usleep(10000);
//     TEST_ASSERT_EQUAL(CC_MASTER_OK, err);

//     TEST_ASSERT_NOT_NULL(test_master_ctx.node_data);
//     TEST_ASSERT_EQUAL(node_cnt, test_master_ctx.node_cnt);

//     for (int i = 0; i < node_cnt; i++) {
//         TEST_ASSERT_EQUAL_HEX8_ARRAY(test_master_ctx.node_data + (i * TEST_PROP_SIZE),
//                                      node_test_grp.node_list[i].node_data, TEST_PROP_SIZE);
//         TEST_ASSERT_EQUAL(rxHeader, node_test_grp.node_list[i].node_ctx.state);
//     }

//     /* Try a write all command */
//     for (int i = 0; i < node_cnt; i++) {
//         memset(node_test_grp.node_list[i].node_data, 0x00, TEST_PROP_SIZE); /* Clear all node data. */
//     }
//     randomize_array(test_master_ctx.node_data, TEST_PROP_SIZE); /* Write all uses data from node 0 */

//     err = cc_master_prop_write_all(&test_master_ctx.master_ctx, property);
//     usleep(10000);
//     TEST_ASSERT_EQUAL(CC_MASTER_ERR_NOT_SUPPORTED, err);

//     for (int i = 0; i < node_cnt; i++) {
//         TEST_ASSERT_EQUAL_HEX8_ARRAY((uint8_t[TEST_PROP_SIZE]) {0x00}, node_test_grp.node_list[i].node_data,
//                                      TEST_PROP_SIZE);
//         TEST_ASSERT_EQUAL(rxHeader, node_test_grp.node_list[i].node_ctx.state);
//     }

//     /* Try a write sequential command */
//     memset(test_master_ctx.node_data, 0xff, TEST_PROP_SIZE * node_cnt);

//     err = cc_master_prop_write_seq(&test_master_ctx.master_ctx, property);
//     usleep(10000);
//     TEST_ASSERT_EQUAL(CC_MASTER_ERR_NOT_SUPPORTED, err);

//     for (int i = 0; i < node_cnt; i++) {
//         TEST_ASSERT_EQUAL_HEX8_ARRAY((uint8_t[TEST_PROP_SIZE]) {0x00}, node_test_grp.node_list[i].node_data,
//                                      TEST_PROP_SIZE);
//         TEST_ASSERT_EQUAL(rxHeader, node_test_grp.node_list[i].node_ctx.state);
//     }
// }

// //----------------------------------------------------------------------------------------------------------------------

// void test_write_only_property(uint8_t node_cnt, cc_prop_id_t property)
// {
//     TEST_ASSERT_TRUE(property == PROP_STATIC_WO || property == PROP_DYNAMIC_WO);
//     TEST_ASSERT_TRUE(255 > node_cnt && node_cnt > 0);

//     cc_master_err_t err = CC_MASTER_OK;

//     /* Initialize master. */
//     setup_cc_master_property_list_handlers();
//     cc_test_master_init(&test_master_ctx);

//     TEST_ASSERT_NULL(test_master_ctx.node_data);
//     TEST_ASSERT_EQUAL(0, test_master_ctx.node_cnt);

//     /* Initialize the nodes. */
//     setup_cc_node_property_list_handlers();
//     cc_test_node_init(&node_test_grp, node_cnt, &test_master_ctx);

//     usleep(100000); // Wait a bit for threads to start

//     /* Try a read all command */
//     for (int i = 0; i < node_cnt; i++) {
//         randomize_array(node_test_grp.node_list[i].node_data, TEST_PROP_SIZE);
//     }

//     err = cc_master_prop_read_all(&test_master_ctx.master_ctx, property);
//     usleep(10000);
//     TEST_ASSERT_EQUAL(CC_MASTER_ERR_NOT_SUPPORTED, err);

//     TEST_ASSERT_NULL(test_master_ctx.node_data);
//     TEST_ASSERT_EQUAL(0, test_master_ctx.node_cnt);

//     /* Try a write all command */
//     test_master_ctx.master_ctx.master.node_cnt_update(&test_master_ctx, node_cnt);
//     randomize_array(test_master_ctx.node_data, TEST_PROP_SIZE); /* Write all uses data from node 0 */

//     err = cc_master_prop_write_all(&test_master_ctx.master_ctx, property);
//     usleep(10000);
//     TEST_ASSERT_EQUAL(CC_MASTER_OK, err);

//     for (int i = 0; i < node_cnt; i++) {
//         TEST_ASSERT_EQUAL_HEX8_ARRAY(test_master_ctx.node_data, node_test_grp.node_list[i].node_data,
//         TEST_PROP_SIZE); TEST_ASSERT_EQUAL(rxHeader, node_test_grp.node_list[i].node_ctx.state);
//     }

//     /* Try a write sequential command */
//     memset(test_master_ctx.node_data, 0xff, TEST_PROP_SIZE * node_cnt);

//     for (int i = 0; i < node_cnt; i++) {
//         memset(node_test_grp.node_list[i].node_data, 0x00, TEST_PROP_SIZE);
//     }

//     err = cc_master_prop_write_seq(&test_master_ctx.master_ctx, property);
//     usleep(10000);
//     TEST_ASSERT_EQUAL(CC_MASTER_OK, err);

//     for (int i = 0; i < node_cnt; i++) {
//         if (i % 2 == 0) {
//             TEST_ASSERT_EQUAL_HEX8_ARRAY(test_master_ctx.node_data + (i * TEST_PROP_SIZE),
//                                          node_test_grp.node_list[i].node_data, TEST_PROP_SIZE);
//         } else {
//             TEST_ASSERT_EQUAL_HEX8_ARRAY((uint8_t[TEST_PROP_SIZE]) {0x00}, node_test_grp.node_list[i].node_data,
//                                          TEST_PROP_SIZE);
//         }
//         TEST_ASSERT_EQUAL(rxHeader, node_test_grp.node_list[i].node_ctx.state);
//     }
// }

//----------------------------------------------------------------------------------------------------------------------

void setUp(void)
{
    // This function is called before each test
}

//----------------------------------------------------------------------------------------------------------------------

void tearDown(void)
{
    /* Deinitialize master and nodes. */
    // cc_test_master_deinit(&test_master_ctx);
    // cc_test_node_deinit(&node_test_grp);
}

//----------------------------------------------------------------------------------------------------------------------

// void test_chain_comm_property_static_rw(void)
// {
//     test_read_write_property(250, PROP_STATIC_RW);
// }

// //----------------------------------------------------------------------------------------------------------------------

// void test_chain_comm_property_dynamic_rw(void)
// {
//     test_read_write_property(250, PROP_DYNAMIC_RW);
// }

// //----------------------------------------------------------------------------------------------------------------------

// void test_chain_comm_property_static_ro(void)
// {
//     test_read_only_property(250, PROP_STATIC_RO);
// }

// //----------------------------------------------------------------------------------------------------------------------

// void test_chain_comm_property_dynamic_ro(void)
// {
//     test_read_only_property(250, PROP_DYNAMIC_RO);
// }

// //----------------------------------------------------------------------------------------------------------------------

// void test_chain_comm_property_static_wo(void)
// {
//     test_write_only_property(250, PROP_STATIC_WO);
// }

// //----------------------------------------------------------------------------------------------------------------------

// void test_chain_comm_property_dynamic_wo(void)
// {
//     test_write_only_property(250, PROP_DYNAMIC_WO);
// }

// //----------------------------------------------------------------------------------------------------------------------

// void test_chain_comm_property_timeout_read_all(void)
// {
//     cc_master_err_t err = CC_MASTER_OK;

//     /* Initialize master. */
//     setup_cc_master_property_list_handlers();
//     cc_test_master_init(&test_master_ctx);

//     /* Initialize the nodes. */
//     setup_cc_node_property_list_handlers();
//     cc_test_node_init(&node_test_grp, 2, &test_master_ctx);

//     usleep(100000); // Wait a bit for threads to start

//     /* Try a read all command */
//     uart_delay_xth_tx(&test_master_ctx.uart, 2, 550);
//     err = cc_master_prop_read_all(&test_master_ctx.master_ctx, PROP_STATIC_RW);
//     usleep(10000);
//     TEST_ASSERT_EQUAL(CC_MASTER_ERR_TIMEOUT, err);
// }

// //----------------------------------------------------------------------------------------------------------------------

// void test_chain_comm_property_timeout_write_seq(void)
// {
//     /* This test has a payload crafted in such a way that the data following the timeout causes another write which
//      * should not happen. V2 protocol should be able to resist this. */
//     size_t node_cnt       = 3;
//     cc_prop_id_t property = PROP_STATIC_RW;
//     cc_master_err_t err   = CC_MASTER_OK;

//     /* Initialize master. */
//     setup_cc_master_property_list_handlers();
//     cc_test_master_init(&test_master_ctx);

//     /* Initialize the nodes. */
//     setup_cc_node_property_list_handlers();
//     cc_test_node_init(&node_test_grp, node_cnt, &test_master_ctx);

//     usleep(100000); // Wait a bit for threads to start

//     /* Try a write all command */
//     test_master_ctx.master_ctx.master.node_cnt_update(&test_master_ctx, node_cnt);

//     memset(test_master_ctx.node_data, 0x00, TEST_PROP_SIZE * node_cnt);

//     cc_msg_header_t header = {
//         .action   = property_writeAll,
//         .property = PROP_STATIC_RW_HALF_SIZE,
//     };
//     test_master_ctx.node_data[4] = header.raw; // Corrupt data so delayed byte is interpreted as a header.
//     memset(test_master_ctx.node_data + 5, 0xFF, (TEST_PROP_SIZE / 2));

//     cc_msg_header_t tail = {
//         .action   = returnCode,
//         .property = 0,
//     };
//     test_master_ctx.node_data[5 + TEST_PROP_SIZE / 2] =
//         tail.raw; // Corrupt data so delayed byte is interpreted as a header.

//     uart_delay_xth_tx(&test_master_ctx.uart, 5, 550);

//     err = cc_master_prop_write_all(&test_master_ctx.master_ctx, property);
//     usleep(10000);

//     for (int i = 0; i < node_cnt; i++) {
//         TEST_ASSERT_EQUAL_HEX8_ARRAY((uint8_t[TEST_PROP_SIZE]) {0x00}, node_test_grp.node_list[i].node_data,
//                                      TEST_PROP_SIZE);
//         TEST_ASSERT_EQUAL(rxHeader, node_test_grp.node_list[i].node_ctx.state);
//     }
//     TEST_ASSERT_EQUAL(CC_MASTER_ERR_FAIL, err);
// }

//----------------------------------------------------------------------------------------------------------------------

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

//--------------------------------------------------------------------------------------------------------------------------------------

void test_cc_header_property_set_get(void)
{
    cc_msg_header_t header = {0};

    cc_header_property_set(&header, 0b11); /* This should only affect raw byte [1]. */
    TEST_ASSERT_EQUAL(0b00000000, header.raw[0]);
    TEST_ASSERT_EQUAL(0b11000000, header.raw[1]);
    TEST_ASSERT_EQUAL(0b00000000, header.raw[2]);
    TEST_ASSERT_EQUAL(0b11, cc_header_property_get(header));

    cc_header_property_set(&header, 0b100); /* This should only affect raw byte [0]. */
    TEST_ASSERT_EQUAL(0b00000001, header.raw[0]);
    TEST_ASSERT_EQUAL(0b00000000, header.raw[1]);
    TEST_ASSERT_EQUAL(0b00000000, header.raw[2]);
    TEST_ASSERT_EQUAL(0b100, cc_header_property_get(header));

    cc_header_property_set(&header, 0b111111); /* All property bits set. */
    TEST_ASSERT_EQUAL(0b00001111, header.raw[0]);
    TEST_ASSERT_EQUAL(0b11000000, header.raw[1]);
    TEST_ASSERT_EQUAL(0b00000000, header.raw[2]);
    TEST_ASSERT_EQUAL(0b111111, cc_header_property_get(header));

    memset(header.raw, 0xFF, sizeof(header.raw));
    cc_header_property_set(&header, 0); /* Clear property */
    TEST_ASSERT_EQUAL(0b11110000, header.raw[0]);
    TEST_ASSERT_EQUAL(0b00111111, header.raw[1]);
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

void test_cc_header_parity_set(void)
{
    /* Configure two identical headers. */
    cc_msg_header_t header_original = {.raw = {0b01100001, 0b00011000, 0b11000000}};
    cc_msg_header_t header_fixed    = {.raw = {0b01100001, 0b00011000, 0b11000000}};

    /* Check that parity is initially wrong. */
    TEST_ASSERT_FALSE(cc_header_parity_check(header_original));

    /* Fix parity. */
    cc_header_parity_set(&header_fixed);

    /* Check that parity is now correct. */
    TEST_ASSERT_TRUE(cc_header_parity_check(header_fixed));

    TEST_ASSERT_EQUAL(header_original.raw[0], header_fixed.raw[0]);
    TEST_ASSERT_EQUAL(header_original.raw[1], header_fixed.raw[1]);
    TEST_ASSERT_EQUAL(header_original.raw[2] & 0b11111110, header_fixed.raw[2] & 0b11111110);

    /* Check that only the parity bit was changed. */
    TEST_ASSERT_TRUE((header_original.raw[2] & 1) != (header_fixed.raw[2] & 1));
}

//----------------------------------------------------------------------------------------------------------------------

void test_cc_master_prop_read(void)
{
    /* Initialize a test master. */
    cc_master_err_t err = CC_MASTER_OK;
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
    // randomize_array(expected_data, sizeof(expected_data));
    memset(expected_data, 0xAA, sizeof(expected_data) - 1);
    expected_data[sizeof(expected_data) - 1] = cc_checksum_calculate(expected_data, sizeof(expected_data) - 1);

    uint8_t tx_payload[100] = {0};
    size_t tx_payload_len   = sizeof(tx_payload) - 3;

    cc_msg_header_t header = {0};
    cc_header_action_set(&header, CC_ACTION_READ);
    cc_header_property_set(&header, PROP_STATIC_RW);
    cc_header_node_cnt_set(&header, 1);
    cc_header_parity_set(&header);

    tx_payload[0] = header.raw[0];
    tx_payload[1] = header.raw[1];
    tx_payload[2] = header.raw[2];
    cc_payload_cobs_encode(tx_payload + 3, &tx_payload_len, expected_data, sizeof(expected_data));

    uart_write(&test_node, tx_payload, tx_payload_len + 3);

    /* Try a read all command */
    err = cc_master_prop_read(&test_master_ctx.master_ctx, PROP_STATIC_RW);

    uint8_t received_data[100] = {0};
    uart_read(&test_master_ctx.uart, received_data, 3);
}

//======================================================================================================================
//                                                         PRIVATE FUNCTIONS
//======================================================================================================================

int main(void)
{
    UNITY_BEGIN();

    // RUN_TEST(test_chain_comm_property_static_rw);
    // RUN_TEST(test_chain_comm_property_dynamic_rw);
    // RUN_TEST(test_chain_comm_property_static_ro);
    // RUN_TEST(test_chain_comm_property_dynamic_ro);
    // RUN_TEST(test_chain_comm_property_static_wo);
    // RUN_TEST(test_chain_comm_property_dynamic_wo);
    // RUN_TEST(test_chain_comm_property_timeout_read_all);
    // RUN_TEST(test_chain_comm_property_timeout_write_seq);

    //---------------------- V2 TESTS ------------------------

    /* Test shared functions. */
    RUN_TEST(test_cc_cobs_encode_decode_ok);
    RUN_TEST(test_cc_cobs_encode_nok);
    RUN_TEST(test_cc_cobs_decode_nok);
    RUN_TEST(test_cc_checksum_update);
    RUN_TEST(test_cc_checksum_calculate);
    RUN_TEST(test_cc_parity_check);
    RUN_TEST(test_cc_node_cnt_set_get);
    RUN_TEST(test_cc_header_action_set_get);
    RUN_TEST(test_cc_header_property_set_get);
    RUN_TEST(test_cc_header_parity_set);

    /* Test Master functions. */
    RUN_TEST(test_cc_master_prop_read);

    return UNITY_END();
}

//----------------------------------------------------------------------------------------------------------------------
