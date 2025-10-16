idf_component_register(
    SRCS 
        "property_handler.c"
        # "property_handlers/offset_property.c"
        # "property_handlers/character_property.c"
        # "property_handlers/character_set_property.c"
        # "property_handlers/command_property.c"
        # "property_handlers/module_info_property.c"
        "property_handlers/firmware_version_property.c"
        # "property_handlers/firmware_update_property.c"
        # "property_handlers/minimum_rotation_property.c"
        # "property_handlers/color_property.c"
        # "property_handlers/motion_property.c"
        # "property_handlers/ir_threshold_property.c"
    INCLUDE_DIRS "include"
    REQUIRES json module)

    