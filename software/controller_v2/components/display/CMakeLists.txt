idf_component_register(
    SRCS 
        "display.c"
    INCLUDE_DIRS 
        "include"
    REQUIRES 
        property_handler webserver module property_handler
)
