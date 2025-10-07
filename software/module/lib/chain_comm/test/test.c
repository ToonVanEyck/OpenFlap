#include "chain_comm_master.h"
#include "chain_comm_master_test.h"
#include "chain_comm_node.h"
#include "chain_comm_node_test.h"
#include "test_properties.h"

#include "unity.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void randomize_array(uint8_t *array, size_t size)
{
    for (size_t i = 0; i < size; i++) {
        array[i] = (uint8_t)(rand() % 256);
    }
}

void test_read_write_property(uint8_t node_cnt, cc_prop_id_t property)
{
    TEST_ASSERT_TRUE(property == PROP_STATIC_RW || property == PROP_DYNAMIC_RW);
    TEST_ASSERT_TRUE(255 > node_cnt && node_cnt > 0);

    cc_master_err_t err = CC_MASTER_OK;

    /* Initialize master. */
    setup_cc_master_property_list_handlers();
    cc_test_master_ctx_t test_master_ctx;
    cc_test_master_init(&test_master_ctx);

    TEST_ASSERT_NULL(test_master_ctx.node_data);
    TEST_ASSERT_EQUAL(0, test_master_ctx.node_cnt);

    /* Initialize the nodes. */
    setup_cc_node_property_list_handlers();

    cc_test_node_group_ctx_t node_test_grp;
    cc_test_node_init(&node_test_grp, node_cnt, &test_master_ctx);

    usleep(100000); // Wait a bit for threads to start

    /* Try a read all command */
    for (int i = 0; i < node_cnt; i++) {
        randomize_array(node_test_grp.node_list[i].node_data, TEST_PROP_SIZE);
    }

    err = cc_master_prop_read_all(&test_master_ctx.master_ctx, property);
    usleep(10000);
    TEST_ASSERT_EQUAL(CC_MASTER_OK, err);

    TEST_ASSERT_NOT_NULL(test_master_ctx.node_data);
    TEST_ASSERT_EQUAL(node_cnt, test_master_ctx.node_cnt);

    for (int i = 0; i < node_cnt; i++) {
        TEST_ASSERT_EQUAL_HEX8_ARRAY(test_master_ctx.node_data + (i * TEST_PROP_SIZE),
                                     node_test_grp.node_list[i].node_data, TEST_PROP_SIZE);
        TEST_ASSERT_EQUAL(rxHeader, node_test_grp.node_list[i].node_ctx.state);
    }

    /* Try a write all command */
    randomize_array(test_master_ctx.node_data, TEST_PROP_SIZE); /* Write all uses data from node 0 */

    err = cc_master_prop_write_all(&test_master_ctx.master_ctx, property);
    usleep(10000);
    TEST_ASSERT_EQUAL(CC_MASTER_OK, err);

    for (int i = 0; i < node_cnt; i++) {
        TEST_ASSERT_EQUAL_HEX8_ARRAY(test_master_ctx.node_data, node_test_grp.node_list[i].node_data, TEST_PROP_SIZE);
        TEST_ASSERT_EQUAL(rxHeader, node_test_grp.node_list[i].node_ctx.state);
    }

    /* Try a write sequential command */
    memset(test_master_ctx.node_data, 0xff, TEST_PROP_SIZE * node_cnt);

    for (int i = 0; i < node_cnt; i++) {
        memset(node_test_grp.node_list[i].node_data, 0x00, TEST_PROP_SIZE);
    }

    err = cc_master_prop_write_seq(&test_master_ctx.master_ctx, property);
    usleep(10000);
    TEST_ASSERT_EQUAL(CC_MASTER_OK, err);

    for (int i = 0; i < node_cnt; i++) {
        if (i % 2 == 0) {
            TEST_ASSERT_EQUAL_HEX8_ARRAY(test_master_ctx.node_data + (i * TEST_PROP_SIZE),
                                         node_test_grp.node_list[i].node_data, TEST_PROP_SIZE);
        } else {
            TEST_ASSERT_EQUAL_HEX8_ARRAY((uint8_t[TEST_PROP_SIZE]) {0x00}, node_test_grp.node_list[i].node_data,
                                         TEST_PROP_SIZE);
        }
        TEST_ASSERT_EQUAL(rxHeader, node_test_grp.node_list[i].node_ctx.state);
    }

    /* Deinitialize nodes. */
    cc_test_node_deinit(&node_test_grp);
}

//----------------------------------------------------------------------------------------------------------------------

void test_read_only_property(uint8_t node_cnt, cc_prop_id_t property)
{
    TEST_ASSERT_TRUE(property == PROP_STATIC_RO || property == PROP_DYNAMIC_RO);
    TEST_ASSERT_TRUE(255 > node_cnt && node_cnt > 0);

    cc_master_err_t err = CC_MASTER_OK;

    /* Initialize master. */
    setup_cc_master_property_list_handlers();
    cc_test_master_ctx_t test_master_ctx;
    cc_test_master_init(&test_master_ctx);

    TEST_ASSERT_NULL(test_master_ctx.node_data);
    TEST_ASSERT_EQUAL(0, test_master_ctx.node_cnt);

    /* Initialize the nodes. */
    setup_cc_node_property_list_handlers();

    cc_test_node_group_ctx_t node_test_grp;
    cc_test_node_init(&node_test_grp, node_cnt, &test_master_ctx);

    usleep(100000); // Wait a bit for threads to start
    /* Try a read all command */
    for (int i = 0; i < node_cnt; i++) {
        randomize_array(node_test_grp.node_list[i].node_data, TEST_PROP_SIZE);
    }

    err = cc_master_prop_read_all(&test_master_ctx.master_ctx, property);
    usleep(10000);
    TEST_ASSERT_EQUAL(CC_MASTER_OK, err);

    TEST_ASSERT_NOT_NULL(test_master_ctx.node_data);
    TEST_ASSERT_EQUAL(node_cnt, test_master_ctx.node_cnt);

    for (int i = 0; i < node_cnt; i++) {
        TEST_ASSERT_EQUAL_HEX8_ARRAY(test_master_ctx.node_data + (i * TEST_PROP_SIZE),
                                     node_test_grp.node_list[i].node_data, TEST_PROP_SIZE);
        TEST_ASSERT_EQUAL(rxHeader, node_test_grp.node_list[i].node_ctx.state);
    }

    /* Try a write all command */
    for (int i = 0; i < node_cnt; i++) {
        memset(node_test_grp.node_list[i].node_data, 0x00, TEST_PROP_SIZE); /* Clear all node data. */
    }
    randomize_array(test_master_ctx.node_data, TEST_PROP_SIZE); /* Write all uses data from node 0 */

    err = cc_master_prop_write_all(&test_master_ctx.master_ctx, property);
    usleep(10000);
    TEST_ASSERT_EQUAL(CC_MASTER_ERR_NOT_SUPPORTED, err);

    for (int i = 0; i < node_cnt; i++) {
        TEST_ASSERT_EQUAL_HEX8_ARRAY((uint8_t[TEST_PROP_SIZE]) {0x00}, node_test_grp.node_list[i].node_data,
                                     TEST_PROP_SIZE);
        TEST_ASSERT_EQUAL(rxHeader, node_test_grp.node_list[i].node_ctx.state);
    }

    /* Try a write sequential command */
    memset(test_master_ctx.node_data, 0xff, TEST_PROP_SIZE * node_cnt);

    err = cc_master_prop_write_seq(&test_master_ctx.master_ctx, property);
    usleep(10000);
    TEST_ASSERT_EQUAL(CC_MASTER_ERR_NOT_SUPPORTED, err);

    for (int i = 0; i < node_cnt; i++) {
        TEST_ASSERT_EQUAL_HEX8_ARRAY((uint8_t[TEST_PROP_SIZE]) {0x00}, node_test_grp.node_list[i].node_data,
                                     TEST_PROP_SIZE);
        TEST_ASSERT_EQUAL(rxHeader, node_test_grp.node_list[i].node_ctx.state);
    }

    /* Deinitialize nodes. */
    cc_test_node_deinit(&node_test_grp);
}

//----------------------------------------------------------------------------------------------------------------------

void test_write_only_property(uint8_t node_cnt, cc_prop_id_t property)
{
    TEST_ASSERT_TRUE(property == PROP_STATIC_WO || property == PROP_DYNAMIC_WO);
    TEST_ASSERT_TRUE(255 > node_cnt && node_cnt > 0);

    cc_master_err_t err = CC_MASTER_OK;

    /* Initialize master. */
    setup_cc_master_property_list_handlers();
    cc_test_master_ctx_t test_master_ctx;
    cc_test_master_init(&test_master_ctx);

    TEST_ASSERT_NULL(test_master_ctx.node_data);
    TEST_ASSERT_EQUAL(0, test_master_ctx.node_cnt);

    /* Initialize the nodes. */
    setup_cc_node_property_list_handlers();

    cc_test_node_group_ctx_t node_test_grp;
    cc_test_node_init(&node_test_grp, node_cnt, &test_master_ctx);

    usleep(100000); // Wait a bit for threads to start

    /* Try a read all command */
    for (int i = 0; i < node_cnt; i++) {
        randomize_array(node_test_grp.node_list[i].node_data, TEST_PROP_SIZE);
    }

    err = cc_master_prop_read_all(&test_master_ctx.master_ctx, property);
    usleep(10000);
    TEST_ASSERT_EQUAL(CC_MASTER_ERR_NOT_SUPPORTED, err);

    TEST_ASSERT_NULL(test_master_ctx.node_data);
    TEST_ASSERT_EQUAL(0, test_master_ctx.node_cnt);

    /* Try a write all command */
    test_master_ctx.master_ctx.master.node_cnt_update(&test_master_ctx, node_cnt);
    randomize_array(test_master_ctx.node_data, TEST_PROP_SIZE); /* Write all uses data from node 0 */

    err = cc_master_prop_write_all(&test_master_ctx.master_ctx, property);
    usleep(10000);
    TEST_ASSERT_EQUAL(CC_MASTER_OK, err);

    for (int i = 0; i < node_cnt; i++) {
        TEST_ASSERT_EQUAL_HEX8_ARRAY(test_master_ctx.node_data, node_test_grp.node_list[i].node_data, TEST_PROP_SIZE);
        TEST_ASSERT_EQUAL(rxHeader, node_test_grp.node_list[i].node_ctx.state);
    }

    /* Try a write sequential command */
    memset(test_master_ctx.node_data, 0xff, TEST_PROP_SIZE * node_cnt);

    for (int i = 0; i < node_cnt; i++) {
        memset(node_test_grp.node_list[i].node_data, 0x00, TEST_PROP_SIZE);
    }

    err = cc_master_prop_write_seq(&test_master_ctx.master_ctx, property);
    usleep(10000);
    TEST_ASSERT_EQUAL(CC_MASTER_OK, err);

    for (int i = 0; i < node_cnt; i++) {
        if (i % 2 == 0) {
            TEST_ASSERT_EQUAL_HEX8_ARRAY(test_master_ctx.node_data + (i * TEST_PROP_SIZE),
                                         node_test_grp.node_list[i].node_data, TEST_PROP_SIZE);
        } else {
            TEST_ASSERT_EQUAL_HEX8_ARRAY((uint8_t[TEST_PROP_SIZE]) {0x00}, node_test_grp.node_list[i].node_data,
                                         TEST_PROP_SIZE);
        }
        TEST_ASSERT_EQUAL(rxHeader, node_test_grp.node_list[i].node_ctx.state);
    }

    /* Deinitialize nodes. */
    cc_test_node_deinit(&node_test_grp);
}

//----------------------------------------------------------------------------------------------------------------------

void setUp(void)
{
    // This function is called before each test
}

void tearDown(void)
{
    // This function is called after each test
}

//----------------------------------------------------------------------------------------------------------------------

void test_chain_comm_property_static_rw(void)
{
    test_read_write_property(250, PROP_STATIC_RW);
}

//----------------------------------------------------------------------------------------------------------------------

void test_chain_comm_property_dynamic_rw(void)
{
    test_read_write_property(250, PROP_DYNAMIC_RW);
}

//----------------------------------------------------------------------------------------------------------------------

void test_chain_comm_property_static_ro(void)
{
    test_read_only_property(250, PROP_STATIC_RO);
}

//----------------------------------------------------------------------------------------------------------------------

void test_chain_comm_property_dynamic_ro(void)
{
    test_read_only_property(250, PROP_DYNAMIC_RO);
}

//----------------------------------------------------------------------------------------------------------------------

void test_chain_comm_property_static_wo(void)
{
    test_write_only_property(250, PROP_STATIC_WO);
}

//----------------------------------------------------------------------------------------------------------------------

void test_chain_comm_property_dynamic_wo(void)
{
    test_write_only_property(250, PROP_DYNAMIC_WO);
}

//----------------------------------------------------------------------------------------------------------------------

//======================================================================================================================
//                                                         PRIVATE FUNCTIONS
//======================================================================================================================

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_chain_comm_property_static_rw);
    RUN_TEST(test_chain_comm_property_dynamic_rw);
    RUN_TEST(test_chain_comm_property_static_ro);
    RUN_TEST(test_chain_comm_property_dynamic_ro);
    RUN_TEST(test_chain_comm_property_static_wo);
    RUN_TEST(test_chain_comm_property_dynamic_wo);

    return UNITY_END();
}

//----------------------------------------------------------------------------------------------------------------------
