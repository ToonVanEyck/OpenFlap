cmake_minimum_required(VERSION 3.14)

project(SplitFlapEmulator   VERSION     1.0
                            LANGUAGES   C
                            DESCRIPTION "Emulator to aid in the development the splitflap chain communication.")

set(CMAKE_BUILD_TYPE "Release")
set(CMAKE_VERBOSE_MAKEFILE OFF)

include_directories("include")
include_directories("../firmware/include")

add_executable(emu_flap main.c ../firmware/chain_comm.c)
add_executable(bm bit_magic.c)