project(flash)
add_library(${PROJECT_NAME} STATIC flash.c)   
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/inc)
target_link_libraries(${PROJECT_NAME} PUBLIC puya_ll)