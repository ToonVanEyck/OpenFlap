#include "config.h"

#define SYMBOL_€ (0x00ac82e2) // € symbol

#define IR_HYSTERESIS (75)
#define IT_THRESHOLD  (750)

#ifdef SET_DEFAULT_CONFIG
/* When SET_DEFAULT_CONFIG is not defined, the default configuration will not be included in the final binary. */
const openflap_config_t __attribute__((section(".config"))) config = {
    .encoder_offset = 0,
    .ir_threshold   = {IT_THRESHOLD - IR_HYSTERESIS, IT_THRESHOLD + IR_HYSTERESIS},
    .base_speed     = 0,
    .symbol_set =
        {
            (uint32_t)' ', (uint32_t)'A', (uint32_t)'B', (uint32_t)'C', (uint32_t)'D', (uint32_t)'E', (uint32_t)'F',
            (uint32_t)'G', (uint32_t)'H', (uint32_t)'I', (uint32_t)'J', (uint32_t)'K', (uint32_t)'L', (uint32_t)'M',
            (uint32_t)'N', (uint32_t)'O', (uint32_t)'P', (uint32_t)'Q', (uint32_t)'R', (uint32_t)'S', (uint32_t)'T',
            (uint32_t)'U', (uint32_t)'V', (uint32_t)'W', (uint32_t)'X', (uint32_t)'Y', (uint32_t)'Z', (uint32_t)'0',
            (uint32_t)'1', (uint32_t)'2', (uint32_t)'3', (uint32_t)'4', (uint32_t)'5', (uint32_t)'6', (uint32_t)'7',
            (uint32_t)'8', (uint32_t)'9', SYMBOL_€,      (uint32_t)'$', (uint32_t)'!', (uint32_t)'?', (uint32_t)'.',
            (uint32_t)',', (uint32_t)':', (uint32_t)'/', (uint32_t)'@', (uint32_t)'#', (uint32_t)'&',
        },
    .ota_completed    = false,
    .minimum_rotation = 1,
    .color            = {0xFFFFFF, 0x000000}, // White on black
    .motion           = {70, 150, 5, 10},
};
#endif