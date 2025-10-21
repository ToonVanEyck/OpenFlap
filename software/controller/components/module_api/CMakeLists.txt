idf_component_register(
    SRCS 
        "module_api.c"
        "module_api_endpoints.c"
        "module_api_firmware_endpoints.c"
    INCLUDE_DIRS 
        "include"
    REQUIRES 
        openflap_property_handlers webserver openflap_module openflap_display
)
