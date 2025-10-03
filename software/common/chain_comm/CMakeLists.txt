cmake_minimum_required (VERSION 3.16.3 FATAL_ERROR)

project(chain_comm  C)
set(CMAKE_C_STANDARD 11)

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake_modules)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_C_FLAGS_DEBUG "-O0 -g")
set(CMAKE_C_FLAGS_RELEASE "-O3")

# Add subdirectories
add_subdirectory(shared)
add_subdirectory(master)
add_subdirectory(node)

# --- Feature toggles ---
option(CC_STATE_STRINGS "Enable human-readable state names." OFF)
option(CC_ERROR_STRINGS "Enable human-readable error strings." OFF)

# Pass compile-time defines
if(CC_STATE_STRINGS)
    target_compile_definitions(${PROJECT_NAME}_shared PUBLIC CC_STATE_STRINGS)
endif()

if(CC_ERROR_STRINGS)
    target_compile_definitions(${PROJECT_NAME}_shared PUBLIC CC_ERROR_STRINGS)
endif()

# Add the test executable
message(STATUS "System processor: ${CMAKE_SYSTEM_PROCESSOR}")
if("${CMAKE_BUILD_TYPE}" STREQUAL "Debug" AND "${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "x86_64")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS_DEBUG}")
    add_library(unity STATIC /opt/unity/src/unity.c)
    target_include_directories(unity PUBLIC /opt/unity/src)
    include(CTest)
    include(CodeCoverage)
    SETUP_TARGET_FOR_COVERAGE_LCOV(
        NAME coverage
        EXECUTABLE ctest -C ${CMAKE_CURRENT_SOURCE_DIR}/CTestTestfile.cmake 
    )
    add_subdirectory(test)
endif()