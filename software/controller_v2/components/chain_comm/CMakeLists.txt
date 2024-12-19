idf_component_register(
    SRCS 
        "chain_comm_esp.c"
        "chain_comm_msg.c"
    INCLUDE_DIRS 
        "include"
    REQUIRES 
        chain_comm_abi esp_driver_uart display module properties
)
