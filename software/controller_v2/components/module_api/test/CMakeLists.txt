idf_component_register(SRCS "test.c"
                       INCLUDE_DIRS .
                       PRIV_REQUIRES unity test_utils module_api module display networking)