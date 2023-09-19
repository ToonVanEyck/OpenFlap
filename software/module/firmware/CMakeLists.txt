set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wpedantic")
# set(CMAKE_C_FLAGS_DEBUG_PRINT "-g")
set(CMAKE_C_FLAGS_RELEASE "-O2") 

file(GLOB_RECURSE MICROCHIP_MDB /opt/microchip/mplabx/*/mplab_platform/bin/mdb.sh )

include_directories("include")
include_directories("${CMAKE_SOURCE_DIR}/../common/include")

add_executable(split_flap_bootloader "btl_main.c" "hardware_configuration.c" "chain_comm.c" "flash.c")
set_target_properties(split_flap_bootloader PROPERTIES LINK_FLAGS "--fill=0x3FFF -mrom=0-07FF")
target_compile_definitions(split_flap_bootloader PUBLIC IS_BTL)

set(number_of_config_pages 4)

math(EXPR config_addr "0x2000 - (${number_of_config_pages} * 32)" OUTPUT_FORMAT HEXADECIMAL)
math(EXPR config_size "(${number_of_config_pages} * 32 * 2 * 14 / 16)" OUTPUT_FORMAT DECIMAL)
math(EXPR checksum_addr "${config_addr} - 1" OUTPUT_FORMAT HEXADECIMAL)
math(EXPR app_end_addr "${checksum_addr} - 1" OUTPUT_FORMAT HEXADECIMAL)
string(SUBSTRING "${checksum_addr}" 2 -1 checksum_addr_striped)
string(SUBSTRING "${app_end_addr}" 2 -1 app_end_addr_striped)

add_compile_definitions(
    CONFIG_START_ADDR=${config_addr}
    CONFIG_SIZE=${config_size}
    CONFIG_PAGES=${number_of_config_pages}
    CHECKSUM_ADDR=${checksum_addr}
    APP_START_ADDR=0x800
    APP_END_ADDR=${app_end_addr})

add_executable(split_flap_firmware "app_main.c" "hardware_configuration.c" "chain_comm.c" "flash.c" ${CMAKE_CURRENT_BINARY_DIR}/version.c)
set_target_properties(split_flap_firmware PROPERTIES LINK_FLAGS "-mrom=0-${checksum_addr_striped} -mchecksum=800-${app_end_addr_striped}@${checksum_addr_striped},algorithm=2,width=-2 -mcodeoffset=0x800")

get_filename_component(HEXMATE "hexmate" REALPATH BASE_DIR "${CMAKE_C_COMPILER}/../../pic/bin")
if(EXISTS "${HEXMATE}")
endif()

math(EXPR cs_byte_addr_h "(${checksum_addr} + 1) * 2" OUTPUT_FORMAT HEXADECIMAL)
math(EXPR cs_byte_addr_l "${cs_byte_addr_h} - 1" OUTPUT_FORMAT HEXADECIMAL)
string(SUBSTRING "${cs_byte_addr_l}" 2 -1 cs_byte_addr_l)
string(SUBSTRING "${cs_byte_addr_h}" 2 -1 cs_byte_addr_h)

add_custom_target(split_flap_complete ALL
    COMMAND ${HEXMATE} -MASK=3F@${cs_byte_addr_l}-${cs_byte_addr_h} ${CMAKE_CURRENT_BINARY_DIR}/split_flap_bootloader.hex ${CMAKE_CURRENT_BINARY_DIR}/split_flap_firmware.hex  ${CMAKE_SOURCE_DIR}/default_characterMap.hex > ${CMAKE_CURRENT_BINARY_DIR}/split_flap_complete.hex
    DEPENDS split_flap_bootloader split_flap_firmware)

add_custom_target(update
    COMMAND curl -s -F 'controller_module_firmware=@${CMAKE_CURRENT_BINARY_DIR}/split_flap_firmware.hex' openflap.local/firmware
    DEPENDS split_flap_complete)

add_custom_target(flash_btl
    COMMAND ${MICROCHIP_MDB} ${PROJECT_SOURCE_DIR}/mdb_flash_btl.txt
    DEPENDS split_flap_bootloader)

add_custom_target(flash
    COMMAND ${MICROCHIP_MDB} ${PROJECT_SOURCE_DIR}/mdb_flash.txt
    DEPENDS split_flap_complete)

add_custom_target(dump
    COMMAND ${MICROCHIP_MDB} ${PROJECT_SOURCE_DIR}/mdb_dump.txt)
