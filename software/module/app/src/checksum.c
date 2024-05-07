#include <stdint.h>

/* Hardcoded checksum, will be replaced by a post build command. */
const uint32_t __attribute__((section(".checksum"))) checksum = 0xdeadbeef;
