idf_component_register(
    SRCS 
        "controller_ota.c"
    INCLUDE_DIRS 
        "include"
    REQUIRES 
        property_handler webserver )
