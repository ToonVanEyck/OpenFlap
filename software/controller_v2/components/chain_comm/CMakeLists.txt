idf_component_register(
    SRCS 
        "chain_comm_esp.c"
        "chain_comm_msg.c"
    INCLUDE_DIRS 
        "include"
    REQUIRES 
        chain_comm_abi esp_driver_uart display module property_handler
)

# target_compile_definitions(${COMPONENT_TARGET} PUBLIC "CHAIN_COMM_DEBUG=1")