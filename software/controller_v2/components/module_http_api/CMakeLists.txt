idf_component_register(
    SRCS 
        "module_http_api.c"
    INCLUDE_DIRS 
        "include"
    REQUIRES 
        property_handler webserver module display
)
