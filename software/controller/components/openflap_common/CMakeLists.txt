idf_component_register(INCLUDE_DIRS ${COMPONENT_DIR}/../../../common/chain_comm_abi/inc/)

include(ExternalProject)
add_library(chain_comm_abi STATIC IMPORTED)
ExternalProject_Add(common_chain_comm_abi
    SOURCE_DIR ${COMPONENT_DIR}/../../../common/chain_comm_abi
    BINARY_DIR ${CMAKE_BINARY_DIR}/chain_comm_abi
    CMAKE_ARGS 
        -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
        -DGENERATE_PROPERTY_NAMES=ON # Generate property names for chain comm properties
    INSTALL_COMMAND ""
)
set_property(TARGET chain_comm_abi PROPERTY IMPORTED_LOCATION ${CMAKE_BINARY_DIR}/chain_comm_abi/libchain_comm_abi.a)
set_property(TARGET chain_comm_abi PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR}/../common/chain_comm_abi/inc)
add_dependencies(chain_comm_abi common_chain_comm_abi)

target_link_libraries(${COMPONENT_LIB} INTERFACE chain_comm_abi)