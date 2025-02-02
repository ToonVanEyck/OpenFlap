set(CHAIN_COMM_ABI_SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/../../../common/chain_comm_abi)

idf_component_register(
    SRCS         ${CHAIN_COMM_ABI_SOURCE_DIR}/chain_comm_abi.c
    INCLUDE_DIRS ${CHAIN_COMM_ABI_SOURCE_DIR}/include
    )
