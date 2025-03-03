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

#define APP_SIZE       (((uint32_t) & __FLASH_APP_SIZE__) + ((uint32_t) & __FLASH_CS_SIZE__))
#define APP_START_PTR  ((uint32_t *)&__FLASH_APP_START__)
#define APP_START_ADDR ((uint32_t) & __FLASH_APP_START__)

#define APP_N_START_PTR(n) (APP_START_PTR + ((n) * APP_SIZE / 4))
#define MAIN_APP           (0)
#define NEW_APP            (1)

#define NVS_SIZE       (((uint32_t) & __FLASH_NVS_SIZE__) + ((uint32_t) & __FLASH_CS_SIZE__))
#define NVS_START_PTR  ((uint32_t *)&__FLASH_NVS_START__)
#define NVS_START_ADDR ((uint32_t) & __FLASH_NVS_START__)