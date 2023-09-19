#ifndef FLASH_H
#define FLASH_H

#include <stdint.h>
#include <xc.h>

// #include "chain_comm.h"

uint32_t clearAndWriteFlash(uint16_t addr, uint8_t *data);
uint8_t validateCheckSum();
uint16_t readFlash(uint16_t address);

#endif
