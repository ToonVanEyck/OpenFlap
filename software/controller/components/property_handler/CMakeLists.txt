idf_component_register(
    SRCS 
        "property_handler.c"
        "property_handlers/calibration_property.c"
        "property_handlers/character_property.c"
        "property_handlers/character_set_property.c"
        "property_handlers/command_property.c"
        "property_handlers/module_info_property.c"
        "property_handlers/firmware_property.c"
    INCLUDE_DIRS "include"
    REQUIRES json chain_comm_abi module)

    