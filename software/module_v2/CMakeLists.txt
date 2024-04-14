cmake_minimum_required (VERSION 3.16.3 FATAL_ERROR)

project(OpenFlapModule  LANGUAGES  C CXX)
set (CMAKE_C_STANDARD 11)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE "Debug")
endif()

add_compile_definitions(${PUYA_CHIP})
set(CMAKE_EXECUTABLE_SUFFIX ".elf")

set(DEBUG_IO_RTT_BACKEND 1 CACHE BOOL "Use RTT as debug input output backend")

add_subdirectory(lib)

# Include Puya BSP
target_include_directories(PuyaBSP PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/app/inc)
target_include_directories(PuyaBSP PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../common/include)

add_subdirectory(app)

# Configure custom "flash" target
add_custom_target(flash
    COMMENT "Flashing software on Puya board"
    COMMAND pyocd load 
            --probe `pyocd list | grep -Po '[0-9a-z]{48}|LU_2022_8888'`
            --pack ${CMAKE_SOURCE_DIR}/Puya.PY32F0xx_DFP.1.1.7.pack 
            --target ${PUYA_CHIP} 
            ${CMAKE_BINARY_DIR}/app/${PROJECT_NAME}.elf
    )
add_dependencies(flash ${PROJECT_NAME})

# Configure custom "rtt" target
add_custom_target(rtt
    COMMENT "Starting rtt listener"
    COMMAND pyocd rtt 
            --probe `pyocd list | grep -Po '[0-9a-z]{48}|LU_2022_8888'`
            --pack ${CMAKE_SOURCE_DIR}/Puya.PY32F0xx_DFP.1.1.7.pack 
            --target ${PUYA_CHIP} 
            -a `grep -Po '.*(0x[0-9a-f]{8}).*_SEGGER_RTT' ${CMAKE_BINARY_DIR}/app/${PROJECT_NAME}.map | grep -Po '0x[0-9a-f]{8}'`
    )
add_dependencies(rtt ${PROJECT_NAME})