idf_component_register(SRCS "test.c"
                       INCLUDE_DIRS .
                       PRIV_REQUIRES unity test_utils property_handler module chain_comm_abi)