set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wpedantic")
# set(CMAKE_C_FLAGS_DEBUG_PRINT "-g")
set(CMAKE_C_FLAGS_RELEASE "-O2") 

file(GLOB_RECURSE MICROCHIP_MDB /opt/microchip/mplabx/*/mplab_platform/bin/mdb.sh )

include_directories("include")

add_executable(split_flap_bootloader "btl_main.c" "chain_comm.c" "flash.c" )
set_target_properties(split_flap_bootloader PROPERTIES LINK_FLAGS "--fill=0x3FFF -mrom=0-07FF")
target_compile_definitions(split_flap_bootloader PUBLIC IS_BTL)

add_executable(split_flap_firmware "app_main.c" "chain_comm.c")
set_target_properties(split_flap_firmware PROPERTIES LINK_FLAGS "-mrom=0-1F9F -mchecksum=800-1F9E@1F9F,algorithm=2,width=-2 -mcodeoffset=0x800")

get_filename_component(HEXMATE "hexmate" REALPATH BASE_DIR "${CMAKE_C_COMPILER}/../../pic/bin")
if(EXISTS "${HEXMATE}")
endif()

add_custom_target(split_flap_complete ALL
    COMMAND ${HEXMATE} -MASK=3F@3F3F-3F40 ${CMAKE_CURRENT_BINARY_DIR}/split_flap_bootloader.hex ${CMAKE_CURRENT_BINARY_DIR}/split_flap_firmware.hex > ${CMAKE_CURRENT_BINARY_DIR}/split_flap_complete.hex
    DEPENDS split_flap_bootloader split_flap_firmware)

add_custom_target(update
    COMMAND curl -s -F 'controller_module_firmware=@${CMAKE_CURRENT_BINARY_DIR}/split_flap_firmware.hex' openflap.local/
    DEPENDS split_flap_complete)

add_custom_target(flash_btl
    COMMAND ${MICROCHIP_MDB} ${PROJECT_SOURCE_DIR}/mdb_flash_btl.txt
    DEPENDS split_flap_bootloader)

add_custom_target(flash
    COMMAND ${MICROCHIP_MDB} ${PROJECT_SOURCE_DIR}/mdb_flash.txt
    DEPENDS split_flap_complete)

add_custom_target(dump
    COMMAND ${MICROCHIP_MDB} ${PROJECT_SOURCE_DIR}/mdb_dump.txt)
