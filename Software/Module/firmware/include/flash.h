#ifndef FLASH_H
#define FLASH_H

#include <xc.h>
#include <stdint.h>

#include "chain_comm.h"

uint32_t clearAndWriteFlash(uint16_t addr, uint8_t *data);
uint8_t validateCheckSum();
void readPage(uint16_t addr, uint16_t *data);

#endif