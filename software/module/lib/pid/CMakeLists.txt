project(pid)

add_library(${PROJECT_NAME} STATIC pid.c)

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/inc)