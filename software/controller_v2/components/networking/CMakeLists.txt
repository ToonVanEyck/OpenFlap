# Get the environment variables
set(NETWORKING_DEFAULT_STA_SSID $ENV{OPENFLAP_DEFAULT_SSID})
set(NETWORKING_DEFAULT_STA_PASS $ENV{OPENFLAP_DEFAULT_PASS})

# Provide default values if the environment variables are not set
if(NOT NETWORKING_DEFAULT_STA_SSID)
    set(NETWORKING_DEFAULT_STA_SSID "default_ssid")
endif()

if(NOT NETWORKING_DEFAULT_STA_PASS)
    set(NETWORKING_DEFAULT_STA_PASS "default_password")
endif()

set(INPUT_FILE "This file is configured by CMake. Do not edit it manually.")

# Configure the networking_default_config.h.in file
configure_file(
    "${CMAKE_CURRENT_LIST_DIR}/include/networking_default_config.h.in"
    "${CMAKE_CURRENT_LIST_DIR}/include/networking_default_config.h"
    @ONLY
)

idf_component_register(
    SRCS "networking.c"
    INCLUDE_DIRS "include"
    PRIV_REQUIRES esp_eth esp_wifi nvs_flash
    )
    
# Check if QEMU is defined and add the define to the compiler flags
if(QEMU)
    target_compile_definitions(${COMPONENT_TARGET} PUBLIC "QEMU=1")
endif()