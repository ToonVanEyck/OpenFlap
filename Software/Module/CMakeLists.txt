# set up the Microchip cross toolchain
set(CMAKE_TOOLCHAIN_FILE external/cmake-microchip/toolchain.cmake)
# set the default MCU model
set(MICROCHIP_MCU PIC16F15225)

cmake_minimum_required(VERSION 3.14)

project(SplitFlap   VERSION     1.0
                    LANGUAGES   C
                    DESCRIPTION "Firmware for a SplitFlap display module")

set(CMAKE_BUILD_TYPE "Release")
set(CMAKE_VERBOSE_MAKEFILE ON)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

include_directories("${CMAKE_SOURCE_DIR}/../common")

find_package(Git REQUIRED)
execute_process(COMMAND ${GIT_EXECUTABLE} describe --tags --dirty --match "V*" OUTPUT_VARIABLE GIT_VERSION_STRING OUTPUT_STRIP_TRAILING_WHITESPACE)
string(REGEX MATCH "^V([0-9]+)\\.([0-9]+)\\.([0-9]+)-?([0-9]*)-?g?([a-z0-9]*)-?(dirty)?" MATCH_DUMMY "${GIT_VERSION_STRING}")
set(CMAKE_PROJECT_VERSION_MAJOR ${CMAKE_MATCH_1})
set(CMAKE_PROJECT_VERSION_MINOR ${CMAKE_MATCH_2})
set(CMAKE_PROJECT_VERSION_PATCH ${CMAKE_MATCH_3})
set(CMAKE_PROJECT_VERSION_TWEAK ${CMAKE_MATCH_4}) # Number of commits after release-tag
set(GIT_VERSION_HASH  ${CMAKE_MATCH_5}) # Hash of current commit
set(GIT_VERSION_IS_DIRTY ${CMAKE_MATCH_6})
set(CMAKE_PROJECT_VERSION "${CMAKE_PROJECT_VERSION_MAJOR}.${CMAKE_PROJECT_VERSION_MINOR}.${CMAKE_PROJECT_VERSION_PATCH}.${CMAKE_PROJECT_VERSION_TWEAK}")

if("${GIT_VERSION_IS_DIRTY}" STREQUAL "")
    set(GIT_VERSION_IS_DIRTY 0)
else()
    set(GIT_VERSION_IS_DIRTY 1)
endif()

if("${GIT_VERSION_COMMITS_AFTER_TAG}" STREQUAL "")
    set(GIT_VERSION_COMMITS_AFTER_TAG 0)
endif()

message(STATUS "${CMAKE_PROJECT_VERSION_MAJOR} ${CMAKE_PROJECT_VERSION_MINOR} ${CMAKE_PROJECT_VERSION_PATCH} ${CMAKE_PROJECT_VERSION_TWEAK} ${GIT_VERSION_HASH} ${GIT_VERSION_IS_DIRTY}")

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/firmware/version.c.in ${CMAKE_CURRENT_BINARY_DIR}/firmware/version.c)

add_subdirectory(firmware)