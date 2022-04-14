#include "flash.h"

uint32_t clearAndWriteFlash(uint16_t addr, uint8_t *data){
    NVMCON1bits.WRERR = 0;
    // set addr
    NVMADR = addr >> 1;
    // set erase
    NVMCON1bits.NVMREGS = 0;
    NVMCON1bits.FREE = 1;
    NVMCON1bits.WREN = 1;
    // Unlock Sequence
    NVMCON2 = 0x55;
    NVMCON2 = 0xAA;
    NVMCON1bits.WR = 1;
    // Earase opperation takes part and cpu halts
    // NVMCON1bits.WREN = 0;
    // set write
    NVMCON1bits.FREE = 0;
    NVMCON1bits.LWLO = 1;
    // NVMCON1bits.WREN = 1;
    for(int i = 0;i<32;i++){
        NVMDAT = ((uint16_t)data[i*2+1]<< 8) | data[i*2];
        if(i<31){
            NVMCON2 = 0x55;
            NVMCON2 = 0xAA;
            NVMCON1bits.WR = 1;
            ++NVMADR;
        }
    }
    NVMCON1bits.LWLO = 0;
    NVMCON2 = 0x55;
    NVMCON2 = 0xAA;
    NVMCON1bits.WR = 1;
    NVMCON1bits.WREN = 0;
    return 1;
}

uint8_t validateCheckSum()
{
    uint16_t start_addr = 0x0800;
    // end_addr /= 2;
	uint16_t calc_checksum = 0;
    uint16_t read_checksum = 0;
    NVMCON1bits.NVMREGS = 0;
	while(start_addr <= 0x1F9E){
        CLRWDT();
        NVMADR = start_addr++;
        NVMCON1bits.RD = 1;
        calc_checksum += NVMDAT;
	}
    NVMADR = 0x1F9F;
    NVMCON1bits.RD = 1;
    read_checksum += NVMDAT;
    return ((calc_checksum & 0x3FFF) == (read_checksum & 0x3FFF));
}
