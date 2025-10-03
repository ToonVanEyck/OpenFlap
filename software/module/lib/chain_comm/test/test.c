#include "chain_comm_master.h"
#include "chain_comm_master_test.h"
#include "chain_comm_node.h"
#include "chain_comm_node_test.h"
#include "test_master_properties.h"
#include "test_node_properties.h"
#include "test_properties.h"

#include "unity.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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

void test_chain_comm_master(void)
{

    /* Initialize master. */
    setup_cc_master_property_list_handlers();
    cc_test_master_ctx_t test_master_ctx;
    cc_test_master_init(&test_master_ctx);

    /* Initialize a node. */
    setup_cc_node_property_list_handlers();
    cc_test_node_ctx_t test_node_ctx;
    cc_test_node_init(0, &test_node_ctx);

    /* Connect the master and node. */
    test_node_ctx.uart.rx_fd   = test_master_ctx.original_rx_fd;
    test_master_ctx.uart.rx_fd = test_node_ctx.original_rx_fd;

    /* Try a read all command */
    cc_master_err_t err = cc_property_read_all(&test_master_ctx.master_ctx, PROP_STATIC_RW);

    /* Try a write all command */
    err = cc_property_write_all(&test_master_ctx.master_ctx, PROP_STATIC_RW);

    /* Try a write sequential command */
    err = cc_property_write_seq(&test_master_ctx.master_ctx, PROP_STATIC_RW);

    cc_test_node_deinit(&test_node_ctx);
}

//----------------------------------------------------------------------------------------------------------------------

//======================================================================================================================
//                                                         PRIVATE FUNCTIONS
//======================================================================================================================

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_chain_comm_master);

    return UNITY_END();
}

//----------------------------------------------------------------------------------------------------------------------
