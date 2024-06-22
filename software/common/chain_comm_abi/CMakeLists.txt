cmake_minimum_required (VERSION 3.16.3 FATAL_ERROR)
project(chain_comm_abi  LANGUAGES  C CXX)
set (CMAKE_C_STANDARD 11)

option(GENERATE_PROPERTY_NAMES "Generate the property names arrays" OFF)

add_library(${PROJECT_NAME} STATIC src/chain_comm_abi.c)   

if(GENERATE_PROPERTY_NAMES)
    target_compile_definitions(${PROJECT_NAME} PUBLIC GENERATE_PROPERTY_NAMES)
endif()

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/inc)