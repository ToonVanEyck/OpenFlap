
add_executable(${Btl}
    src/py32f0xx_it.c 
    src/py32f0xx_hal_msp.c 
    src/bootloader.c
)

target_link_libraries(${Btl} PUBLIC debug_io)
target_link_libraries(${Btl} PUBLIC $<TARGET_OBJECTS:PuyaBSP>)
target_include_directories(${Btl} PRIVATE $<TARGET_PROPERTY:PuyaBSP,INTERFACE_INCLUDE_DIRECTORIES>)

set_target_properties(${Btl} PROPERTIES LINK_FLAGS "-T ${CMAKE_CURRENT_SOURCE_DIR}/linker_script_btl.ld")

add_custom_command(TARGET ${Btl}
    POST_BUILD
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Generating hex file"
    COMMAND ${CMAKE_OBJCOPY} ARGS -O ihex ${Btl}.elf ${Btl}.hex
)