add_library(${PROJECT_NAME}_shared chain_comm_shared.c
)

target_include_directories(${PROJECT_NAME}_shared
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/inc
)