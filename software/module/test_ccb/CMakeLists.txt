# Set executable sources
add_executable(${App} 
    src/main.c 
    ../app/src/openflap_hal.c
    ../app/src/py32f0xx_it.c 
    ../app/src/chain_comm.c
    ../app/src/property_handlers.c
    ../app/src/openflap.c
    ../app/src/default_config.c
    ../app/src/checksum.c
    ../app/src/debug_term.c
)

# Get the version using git describe
execute_process(
    COMMAND git describe --tag --dirty
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_VERSION
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

if(NOT GIT_VERSION)
    set(GIT_VERSION "undefined")
endif()


# Add the version as a compile definition
target_compile_definitions(${App} PUBLIC GIT_VERSION="${GIT_VERSION}")

target_compile_definitions(${App} PUBLIC
    # CHAIN_COMM_DEBUG=1          # Print debug info for chain communication.
    SET_DEFAULT_CONFIG          # provide a default configuration in the binary.
)

target_link_libraries(${App} PUBLIC puya_ll debug_io flash pid rbuff openflap_properties uart_driver interpolation) 

target_include_directories(${App} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../app/inc)

set_target_properties(${App} PROPERTIES LINK_FLAGS "-T ${CMAKE_CURRENT_SOURCE_DIR}/../app/linker_script_app.ld")
    
# Generate hex and bin files
add_custom_command(TARGET ${App}
    POST_BUILD
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Generating binaries"
    # Create temporary hex file
    COMMAND ${CMAKE_OBJCOPY} ARGS 
        -O ihex ${App}.elf temp.hex
    # Insert the checksum in the hex file
    COMMAND srec_cat ARGS 
        temp.hex -Intel 
        -crop 0x08001000 0x08007FFC 
        -fill 0xFF 0x08001000 0x08007FFC
        -STM32 0x08007FFC
        # -line-length=44
        -o ${CMAKE_BINARY_DIR}/bin/${App}.hex -Intel
    # Create binary file
    COMMAND ${CMAKE_OBJCOPY} ARGS
        --input-target=ihex 
        --output-target=binary 
        ${CMAKE_BINARY_DIR}/bin/${App}.hex
        ${CMAKE_BINARY_DIR}/bin/${App}.bin
    # Extracting config from hex file
    COMMAND srec_cat ARGS 
        temp.hex -Intel 
        -crop 0x0800F000 0x0800FFFF 
        -o ${CMAKE_BINARY_DIR}/bin/${PROJECT_NAME}_Cfg.hex -Intel
)
