# set up the Microchip cross toolchain
set(CMAKE_TOOLCHAIN_FILE external/cmake-microchip/toolchain.cmake)
# set the default MCU model
set(MICROCHIP_MCU PIC16F15225)

cmake_minimum_required(VERSION 3.14)

project(SplitFlap   VERSION     1.0
                    LANGUAGES   C
                    DESCRIPTION "Firmware for a SplitFlap display module")

set(CMAKE_BUILD_TYPE "Release")
set(CMAKE_VERBOSE_MAKEFILE OFF)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

include(GetGitRevisionDescription)
git_describe(VERSION --tags --dirty=-d)
#parse the version information into pieces.
string(REGEX REPLACE "^v([0-9]+)\\..*" "\\1"                GIT_VERSION_MAJOR "${VERSION}")
string(REGEX REPLACE "^v[0-9]+\\.([0-9]+).*" "\\1"          GIT_VERSION_MINOR "${VERSION}")
string(REGEX REPLACE "^v[0-9]+\\.[0-9]+\\.([0-9]+).*" "\\1" GIT_VERSION_PATCH "${VERSION}")
string(REGEX REPLACE "^v[0-9]+\\.[0-9]+\\.[0-9]+(.*)" "\\1" GIT_VERSION_SHA1  "${VERSION}")

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/firmware/version.c.in ${CMAKE_CURRENT_BINARY_DIR}/firmware/version.c)

include(ExternalProject)
ExternalProject_Add(emulator    SOURCE_DIR      ${CMAKE_SOURCE_DIR}/flap_emulator 
                                BINARY_DIR      ${CMAKE_BINARY_DIR}/flap_emulator 
                                BUILD_ALWAYS    true
                                INSTALL_COMMAND "")

add_subdirectory(firmware)