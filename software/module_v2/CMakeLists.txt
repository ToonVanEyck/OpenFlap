cmake_minimum_required (VERSION 3.16.3 FATAL_ERROR)

project(OpenFlap_Module  LANGUAGES  C CXX)
set (CMAKE_C_STANDARD 11)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE "Debug")
endif()

add_compile_definitions(${PUYA_CHIP})
set(CMAKE_EXECUTABLE_SUFFIX ".elf")

file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(DEBUG_IO_RTT_BACKEND 1 CACHE BOOL "Use RTT as debug input output backend")

add_subdirectory(lib)

# Include Puya BSP
target_include_directories(PuyaBSP PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/app/inc)
target_include_directories(PuyaBSP PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/btl/inc)
target_include_directories(PuyaBSP PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../common/include)

set(App ${PROJECT_NAME}_App)
set(Btl ${PROJECT_NAME}_Btl)
add_subdirectory(app)
add_subdirectory(btl)

add_custom_command(OUTPUT ${PROJECT_NAME}_BtlAppCfg.hex
    COMMENT "Merging Bootloader, Application and Configuration"
    COMMAND srec_cat 
            bin/${PROJECT_NAME}_App.hex -Intel 
            bin/${PROJECT_NAME}_Cfg.hex -Intel 
            btl/${Btl}.hex -Intel 
            -o bin/${PROJECT_NAME}_BtlAppCfg.hex -Intel -line-length=44
    DEPENDS ${App} ${Btl}
    )
add_custom_target(BtlAppCfg_hex ALL DEPENDS ${PROJECT_NAME}_BtlAppCfg.hex)

add_custom_command(OUTPUT ${PROJECT_NAME}_AppApp.hex
    COMMENT "Duplicating Application with offset"
    COMMAND srec_cat 
            bin/${PROJECT_NAME}_App.hex -Intel 
            -offset 0x7000
            bin/${PROJECT_NAME}_App.hex -Intel 
            -o bin/${PROJECT_NAME}_AppApp.hex -Intel -line-length=44
    DEPENDS ${App} ${Btl}
    )
add_custom_target(AppApp_hex ALL DEPENDS ${PROJECT_NAME}_AppApp.hex)

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -L ${CMAKE_SOURCE_DIR}" CACHE INTERNAL "Linker options")

# Configure custom "flash" target
add_custom_target(flash_target
    COMMENT "Flashing bootloader + App on Puya board"
    COMMAND pyocd load 
            --probe `pyocd list | grep -Po '[0-9a-z]{48}|LU_2022_8888'`
            --pack ${CMAKE_SOURCE_DIR}/Puya.PY32F0xx_DFP.1.1.7.pack 
            --target ${PUYA_CHIP} 
            ${CMAKE_BINARY_DIR}/bin/${PROJECT_NAME}_BtlAppCfg.hex
    )
add_dependencies(flash_target BtlAppCfg_hex)

# Configure custom "flash" target
add_custom_target(flash_app
    COMMENT "Flashing bootloader + App on Puya board"
    COMMAND pyocd load 
            --probe `pyocd list | grep -Po '[0-9a-z]{48}|LU_2022_8888'`
            --pack ${CMAKE_SOURCE_DIR}/Puya.PY32F0xx_DFP.1.1.7.pack 
            --target ${PUYA_CHIP} 
            ${CMAKE_BINARY_DIR}/bin/${PROJECT_NAME}_AppApp.hex
    )
add_dependencies(flash_target AppApp_hex)

# Configure custom "rtt" target
add_custom_target(rtt
    COMMENT "Starting rtt listener"
    COMMAND pyocd rtt 
            #--probe `pyocd list | grep -Po '[0-9a-z]{48}|LU_2022_8888'`
            --pack ${CMAKE_SOURCE_DIR}/Puya.PY32F0xx_DFP.1.1.7.pack 
            --target ${PUYA_CHIP} 
            -a `grep -Po '.*(0x[0-9a-f]{8}).*_SEGGER_RTT' ${CMAKE_BINARY_DIR}/app/${PROJECT_NAME}.map | grep -Po '0x[0-9a-f]{8}'`
    )
# add_dependencies(rtt ${App})