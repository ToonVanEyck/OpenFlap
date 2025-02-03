cmake_minimum_required (VERSION 3.16.3 FATAL_ERROR)
project(chain_comm_abi  LANGUAGES  C CXX)
set (CMAKE_C_STANDARD 11)

add_library(${PROJECT_NAME} STATIC chain_comm_abi.c)   

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)