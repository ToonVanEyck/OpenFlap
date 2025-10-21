idf_component_register(
    SRCS openflap_property_handlers.c
    INCLUDE_DIRS "include"
    REQUIRES json openflap_module openflap_display
)

    