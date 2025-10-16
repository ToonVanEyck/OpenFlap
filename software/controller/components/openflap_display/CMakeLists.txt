idf_component_register(
    SRCS 
        "openflap_display.c"
    INCLUDE_DIRS 
        "include"
    REQUIRES 
        property_handler webserver module openflap_cc_master 
)
