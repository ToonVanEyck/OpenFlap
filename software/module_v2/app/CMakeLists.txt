# Set executable sources
add_executable(${PROJECT_NAME} 
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

target_compile_definitions(${PROJECT_NAME} PUBLIC
    DO_GENERATE_PROPERTY_NAMES # Generate property names for chain comm properties
    DO_GENERATE_STATE_NAMES # Generate state names for chain comm fsm
    SET_DEFAULT_CONFIG # provide a default configuration in the binary
)

target_link_libraries(${PROJECT_NAME} PUBLIC debug_io)
target_link_libraries(${PROJECT_NAME} PUBLIC $<TARGET_OBJECTS:PuyaBSP>)

target_include_directories(${PROJECT_NAME} PRIVATE $<TARGET_PROPERTY:PuyaBSP,INTERFACE_INCLUDE_DIRECTORIES>)

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -T ${CMAKE_CURRENT_SOURCE_DIR}/LinkerScript.ld" CACHE INTERNAL "Linker options")