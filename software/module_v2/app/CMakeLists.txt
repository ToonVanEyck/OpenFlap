# Set executable sources
add_executable(${App} 
    src/py32f0xx_it.c 
    src/py32f0xx_hal_msp.c 
    src/main.c 
    src/config.c 
    src/flash.c 
    src/chain_comm.c
    src/property_handlers.c
    src/openflap.c
    src/default_config.c
    src/checksum.c
)

target_compile_definitions(${App} PUBLIC
    DO_GENERATE_PROPERTY_NAMES # Generate property names for chain comm properties
    DO_GENERATE_STATE_NAMES # Generate state names for chain comm fsm
    SET_DEFAULT_CONFIG # provide a default configuration in the binary
)

target_link_libraries(${App} PUBLIC debug_io)
target_link_libraries(${App} PUBLIC $<TARGET_OBJECTS:PuyaBSP>)
target_include_directories(${App} PRIVATE $<TARGET_PROPERTY:PuyaBSP,INTERFACE_INCLUDE_DIRECTORIES>)

set_target_properties(${App} PROPERTIES LINK_FLAGS "-T ${CMAKE_CURRENT_SOURCE_DIR}/linker_script_app.ld")

add_custom_command(TARGET ${App}
    POST_BUILD
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Generating hex file"
    COMMAND ${CMAKE_OBJCOPY} ARGS -O ihex ${App}.elf ${App}.hex
)