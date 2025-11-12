idf_component_register(
    SRCS 
        "openflap_module_property.c"
        "openflap_module.c"
    INCLUDE_DIRS 
        "include"
    REQUIRES 
        webserver openflap_mdl_master
)
