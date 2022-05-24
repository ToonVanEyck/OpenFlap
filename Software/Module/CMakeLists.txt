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
git_describe(VERSION --tags --dirty=*)
#parse the version information into pieces.

string(REGEX REPLACE "^v([0-9]+)\\.([0-9]+)\\.([0-9]+)-?([0-9]?)-?g?([a-z0-9]*)(\\*?)" "\\1" GIT_VERSION_MAJOR "${VERSION}")
string(REGEX REPLACE "^v([0-9]+)\\.([0-9]+)\\.([0-9]+)-?([0-9]?)-?g?([a-z0-9]*)(\\*?)" "\\2" GIT_VERSION_MINOR "${VERSION}")
string(REGEX REPLACE "^v([0-9]+)\\.([0-9]+)\\.([0-9]+)-?([0-9]?)-?g?([a-z0-9]*)(\\*?)" "\\3" GIT_VERSION_PATCH "${VERSION}")
string(REGEX REPLACE "^v([0-9]+)\\.([0-9]+)\\.([0-9]+)-?([0-9]?)-?g?([a-z0-9]*)(\\*?)" "\\4" GIT_VERSION_COMMITS_AFTER_TAG  "${VERSION}")
string(REGEX REPLACE "^v([0-9]+)\\.([0-9]+)\\.([0-9]+)-?([0-9]?)-?g?([a-z0-9]*)(\\*?)" "\\5" GIT_VERSION_HASH  "${VERSION}")
string(REGEX REPLACE "^v([0-9]+)\\.([0-9]+)\\.([0-9]+)-?([0-9]?)-?g?([a-z0-9]*)(\\*?)" "\\6" GIT_VERSION_IS_DIRTY  "${VERSION}")
string(REPLACE "*" "1" GIT_VERSION_IS_DIRTY ${GIT_VERSION_IS_DIRTY})

message(STATUS "${GIT_VERSION_MAJOR} ${GIT_VERSION_MINOR} ${GIT_VERSION_PATCH} ${GIT_VERSION_COMMITS_AFTER_TAG} ${GIT_VERSION_HASH} ${GIT_VERSION_IS_DIRTY}")

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/firmware/version.c.in ${CMAKE_CURRENT_BINARY_DIR}/firmware/version.c)

add_subdirectory(firmware)