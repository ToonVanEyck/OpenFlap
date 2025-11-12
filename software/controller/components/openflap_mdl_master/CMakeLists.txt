set(CHAIN_COMM_SRC_DIR ${CMAKE_CURRENT_LIST_DIR}/../../../common/madelink)
set(OPENFLAP_PROPERTIES_SRC_DIR ${CMAKE_CURRENT_LIST_DIR}/../../../common/openflap_properties)

idf_component_register(
    SRCS 
        ${CHAIN_COMM_SRC_DIR}/shared/madelink_shared.c
        ${CHAIN_COMM_SRC_DIR}/master/madelink_master.c
        ${OPENFLAP_PROPERTIES_SRC_DIR}/openflap_properties.c
        "openflap_mdl_master.c"
        "openflap_mdl_master_uart.c"
    INCLUDE_DIRS 
        ${CHAIN_COMM_SRC_DIR}/shared/inc
        ${CHAIN_COMM_SRC_DIR}/master/inc
        ${OPENFLAP_PROPERTIES_SRC_DIR}/inc
        "include"
    REQUIRES 
        esp_driver_uart esp_driver_gpio esp_rom 
)

idf_build_set_property(COMPILE_DEFINITIONS "MDL_HANDLER_ALTERNATE_ENABLE=1" APPEND)
idf_build_set_property(COMPILE_DEFINITIONS "MDL_ERROR_STRINGS=1" APPEND)
idf_build_set_property(COMPILE_DEFINITIONS "MDL_STATE_STRINGS=1" APPEND)