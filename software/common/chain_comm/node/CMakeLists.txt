add_library(${PROJECT_NAME}_node STATIC chain_comm_node.c)
target_include_directories(${PROJECT_NAME}_node PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/inc)

target_link_libraries(${PROJECT_NAME}_node
    PUBLIC
        ${PROJECT_NAME}_shared
)