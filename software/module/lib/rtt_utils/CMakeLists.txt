project(rtt_utils)

add_library(${PROJECT_NAME} STATIC rtt_utils.c syscalls.c)

include(FetchContent)
FetchContent_Declare(
    jlink_rtt
    GIT_REPOSITORY https://github.com/jmacheta/jlink_rtt-cmake
    GIT_TAG main
)

FetchContent_MakeAvailable(jlink_rtt)
target_include_directories(jlink_rtt PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/inc) # Use custom RTT config.

target_link_libraries(${PROJECT_NAME} PUBLIC jlink_rtt)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/inc)