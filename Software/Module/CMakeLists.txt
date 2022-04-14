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

include(ExternalProject)
ExternalProject_Add(emulator    SOURCE_DIR      ${CMAKE_SOURCE_DIR}/flap_emulator 
                                BINARY_DIR      ${CMAKE_BINARY_DIR}/flap_emulator 
                                BUILD_ALWAYS    true
                                INSTALL_COMMAND "")

add_subdirectory(firmware)