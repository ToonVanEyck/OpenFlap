project(debug_io)

add_library(${PROJECT_NAME} STATIC debug_io_rtt.c)

if(DEBUG_IO_RTT_BACKEND)
    # Get RTT from github.
    include(FetchContent)
    FetchContent_Declare(
        jlink_rtt
        GIT_REPOSITORY https://github.com/jmacheta/jlink_rtt-cmake
        GIT_TAG main
        )
        FetchContent_MakeAvailable(jlink_rtt)
        target_include_directories(jlink_rtt PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/inc) # Use custom RTT config.
        target_link_libraries(${PROJECT_NAME} PRIVATE jlink_rtt)
        add_definitions(-DRTT_BACKEND)
endif()
    
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/inc)