#pragma once
#include <stdint.h>

extern uint32_t __FLASH_BOOT_START__;
extern uint32_t __FLASH_BOOT_SIZE__;
extern uint32_t __FLASH_APP_START__;
extern uint32_t __FLASH_APP_SIZE__;
extern uint32_t __FLASH_CS_START__;
extern uint32_t __FLASH_CS_SIZE__;
extern uint32_t __FLASH_NVS_START__;
extern uint32_t __FLASH_NVS_SIZE__;

#define APP_SIZE (((uint32_t) & __FLASH_APP_SIZE__) / 4 + ((uint32_t) & __FLASH_CS_SIZE__) / 4)
#define APP_START_PTR ((uint32_t *)&__FLASH_APP_START__)
#define APP_START_ADDR ((uint32_t) & __FLASH_APP_START__)

#define NVS_SIZE (((uint32_t) & __FLASH_NVS_SIZE__) / 4 + ((uint32_t) & __FLASH_CS_SIZE__) / 4)
#define NVS_START_PTR ((uint32_t *)&__FLASH_NVS_START__)
#define NVS_START_ADDR ((uint32_t) & __FLASH_NVS_START__)