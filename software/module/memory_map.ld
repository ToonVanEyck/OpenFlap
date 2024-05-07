/* Memory Map
 *
 *
 *   +===============+ 0x08000000
 *   |  Bootloader   | 
 *   +---------------+ 0x08001000
 *   |               |
 *   | Application 1 |
 *   |               |
 *   +---------------+ 0x08007FFC
 *   |   Checksum 1  |
 *   +===============+ 0x08008000
 *   |               |
 *   | Application 2 |
 *   |               |
 *   +---------------+ 0x0800EFFC
 *   |   Checksum 2  |
 *   +===============+ 0x0800F000
 *   |     Config    |
 *   +===============+ 0x0800FFFF
 *
 *
 *
 */

MEMORY
{
  RAM (xrw)      : ORIGIN = 0x20000000, LENGTH = 8K  /* TODO: CHANGE TO 8K RAM */
  FLASH_BTL (rx) : ORIGIN = 0x08000000, LENGTH = 4K  /* Bootloader */
  FLASH_APP (rx) : ORIGIN = 0x08001000, LENGTH = 28K - 4 /* [ 64K (total) - 4K (nvs) - 4K bootloader ] / 2 = 28K - 4 (Checksum) */ 
  FLASH_CS(r)    : ORIGIN = 0x08007FFC, LENGTH = 4  /* Checksum sector */
  FLASH_NVS (r)  : ORIGIN = 0x0800F000, LENGTH = 4K  /* Flash sector used to store OpenFlap configuration. */
}

__FLASH_BTL_START__  = ORIGIN(FLASH_BTL);
__FLASH_BTL_SIZE__   = LENGTH(FLASH_BTL);
__FLASH_APP_START__  = ORIGIN(FLASH_APP);
__FLASH_APP_SIZE__   = LENGTH(FLASH_APP);
__FLASH_CS_START__   = ORIGIN(FLASH_CS);
__FLASH_CS_SIZE__    = LENGTH(FLASH_CS);
__FLASH_NVS_START__  = ORIGIN(FLASH_NVS);
__FLASH_NVS_SIZE__   = LENGTH(FLASH_NVS);
