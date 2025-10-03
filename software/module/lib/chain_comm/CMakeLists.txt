project(chain_comm C)

# Use standard C
set(CMAKE_C_STANDARD 99)

# Add subdirectories
add_subdirectory(shared)
add_subdirectory(master)
add_subdirectory(node)

# Add the test executable
if("${CMAKE_BUILD_TYPE}" STREQUAL "HostTest")
    add_subdirectory(test)
endif()