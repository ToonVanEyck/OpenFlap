project(config)
add_library(${PROJECT_NAME} STATIC config.c)   
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/inc)
target_link_libraries(${PROJECT_NAME} PRIVATE flash debug_io)