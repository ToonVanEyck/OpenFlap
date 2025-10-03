add_library(${PROJECT_NAME}_master STATIC chain_comm_master.c)
target_include_directories(${PROJECT_NAME}_master PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/inc)

target_link_libraries(${PROJECT_NAME}_master
    PUBLIC
        ${PROJECT_NAME}_shared
)