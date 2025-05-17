project(stepper_driver)
add_library(${PROJECT_NAME} STATIC stepper_driver.c)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/inc)
target_link_libraries(${PROJECT_NAME} PUBLIC debug_io)

if(BUILD_TESTING)
    add_subdirectory(test)
endif()