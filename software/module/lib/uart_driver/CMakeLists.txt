project(uart_driver)
add_library(${PROJECT_NAME} STATIC uart_driver.c)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/inc)
target_link_libraries(${PROJECT_NAME} PUBLIC rbuff puya_ll)