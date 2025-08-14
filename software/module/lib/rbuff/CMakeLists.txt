project(rbuff)
add_library(${PROJECT_NAME} STATIC rbuff.c)   
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/inc)

# Add the test executable
if("${CMAKE_BUILD_TYPE}" STREQUAL "HostTest")
    add_subdirectory(test)
endif()