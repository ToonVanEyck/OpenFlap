
add_executable(${Btl}
    src/bootloader.c
    src/py32f0xx_it.c 
)

target_link_libraries(${Btl} PUBLIC puya_ll flash)

target_include_directories(${Btl} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/inc)

set_target_properties(${Btl} PROPERTIES LINK_FLAGS "-T ${CMAKE_CURRENT_SOURCE_DIR}/linker_script_btl.ld")

add_custom_command(TARGET ${Btl}
    POST_BUILD
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Generating hex file"
    COMMAND ${CMAKE_OBJCOPY} ARGS -O ihex ${Btl}.elf ${Btl}.hex
)