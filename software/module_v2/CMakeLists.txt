cmake_minimum_required (VERSION 3.16.3 FATAL_ERROR)

project(OpenFlapModule  LANGUAGES  C CXX)
set (CMAKE_C_STANDARD 11)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE "Debug")
endif()

add_compile_definitions(${PUYA_CHIP})
set(CMAKE_EXECUTABLE_SUFFIX ".elf")

# Get RTT from github.
include(FetchContent)
FetchContent_Declare(
    jlink_rtt
    GIT_REPOSITORY https://github.com/jmacheta/jlink_rtt-cmake
    GIT_TAG main
)
FetchContent_MakeAvailable(jlink_rtt)
target_include_directories(jlink_rtt PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/app/inc)

# Include Puya BSP
add_subdirectory(puya_libs)
target_include_directories(PuyaBSP PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/app/inc)
target_include_directories(PuyaBSP PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../common/include)

# Set executable sources
add_executable(${PROJECT_NAME} 
    app/src/py32f0xx_it.c 
    app/src/py32f0xx_hal_msp.c 
    app/src/main.c 
    app/src/config.c 
    app/src/flash.c 
    app/src/chain_comm.c
    app/src/property_handlers.c
    app/src/openflap.c
)

target_compile_definitions(${PROJECT_NAME} PUBLIC
    DO_GENERATE_PROPERTY_NAMES
    DO_GENERATE_STATE_NAMES
)

target_link_libraries(${PROJECT_NAME} PUBLIC jlink_rtt)
target_link_libraries(${PROJECT_NAME} PUBLIC $<TARGET_OBJECTS:PuyaBSP>)
target_include_directories(${PROJECT_NAME} PRIVATE $<TARGET_PROPERTY:PuyaBSP,INTERFACE_INCLUDE_DIRECTORIES>)

# Configure custom "flash" target
add_custom_target(flash
    COMMENT "Flashing software on Puya board"
    COMMAND pyocd load 
            --probe `pyocd list | grep -Po '[0-9a-z]{48}|LU_2022_8888'`
            --pack ${CMAKE_SOURCE_DIR}/Puya.PY32F0xx_DFP.1.1.7.pack 
            --target ${PUYA_CHIP} 
            ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.elf
    )
add_dependencies(flash ${PROJECT_NAME})

# Configure custom "rtt" target
add_custom_target(rtt
    COMMENT "Starting rtt listener"
    COMMAND pyocd rtt 
            --probe `pyocd list | grep -Po '[0-9a-z]{48}|LU_2022_8888'`
            --pack ${CMAKE_SOURCE_DIR}/Puya.PY32F0xx_DFP.1.1.7.pack 
            --target ${PUYA_CHIP} 
            -a `grep -Po '.*(0x[0-9a-f]{8}).*_SEGGER_RTT' ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.map | grep -Po '0x[0-9a-f]{8}'`
    )
add_dependencies(rtt ${PROJECT_NAME})