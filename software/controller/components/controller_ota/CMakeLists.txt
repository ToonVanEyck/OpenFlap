idf_component_register(
    SRCS 
        "controller_ota.c"
    INCLUDE_DIRS 
        "include"
    REQUIRES 
        webserver app_update)
