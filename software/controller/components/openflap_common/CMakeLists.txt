set(CHAIN_COMM_ABI_SOURCE_DIR ${CMAKE_SOURCE_DIR}/../common/chain_comm_abi)

MESSAGE(STATUS ${CHAIN_COMM_ABI_SOURCE_DIR})
idf_component_register(
    SRCS         ${CHAIN_COMM_ABI_SOURCE_DIR}/src/chain_comm_abi.c
    INCLUDE_DIRS ${CHAIN_COMM_ABI_SOURCE_DIR}/inc
)
