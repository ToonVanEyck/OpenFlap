idf_component_register(
    SRCS 
        "openflap_display.c"
    INCLUDE_DIRS 
        "include"
    REQUIRES 
        openflap_property_handlers webserver openflap_module openflap_cc_master 
)
