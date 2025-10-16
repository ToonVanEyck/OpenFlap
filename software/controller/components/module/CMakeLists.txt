idf_component_register(
    SRCS 
        "module_property.c"
        "module.c"
    INCLUDE_DIRS 
        "include"
    REQUIRES 
        webserver openflap_properties
)
