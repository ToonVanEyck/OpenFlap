#include "property_handlers.h"
#include "config.h"
#include "debug_io.h"
#include "flash.h"
#include "memory_map.h"
#include "peripherals.h"

#include <stdint.h>
#include <string.h>

static openflap_ctx_t *openflap_ctx = NULL; /* Set during initialization. */

extern uint32_t checksum;

void property_firmware_set(uint8_t *buf, uint16_t *size)
{
    uint32_t addr_base   = (uint32_t)(APP_START_PTR + (NEW_APP * APP_SIZE / 4));
    uint32_t addr_offset = ((uint32_t)buf[0] << 8 | (uint32_t)buf[1]) * FLASH_PAGE_SIZE;
    uint32_t addr        = addr_base + addr_offset;
    flashWrite(addr, (buf + 2), FLASH_PAGE_SIZE);
    if (addr_offset + FLASH_PAGE_SIZE == APP_SIZE) {
        openflap_ctx->config.ota_completed = true;
        openflap_ctx->store_config         = true;
    }
}

void property_firmware_get(uint8_t *buf, uint16_t *size)
{
    *size = strlen(GIT_VERSION);
    strncpy((char *)buf, GIT_VERSION, CHAIN_COM_MAX_LEN - 5); // Reserve 4 bytes for checksum
    // append checksum bytes
    buf[(*size)++] = (checksum >> 24) & 0xFF;
    buf[(*size)++] = (checksum >> 16) & 0xFF;
    buf[(*size)++] = (checksum >> 8) & 0xFF;
    buf[(*size)++] = checksum & 0xFF;
}

void property_command_set(uint8_t *buf, uint16_t *size)
{
    switch (buf[0]) {
        case CMD_REBOOT:
            /* Reboot is handled later to allow graceful end of communication. */
            openflap_ctx->reboot = true;
            break;
        default:
            break;
    }
}

void property_module_info_get(uint8_t *buf, uint16_t *size)
{
    module_info_property_t module_info = {0};
    module_info.column_end             = is_column_end();
    module_info.type                   = MODULE_TYPE_SPLITFLAP;
    buf[0]                             = module_info.raw;
}

void property_character_set_set(uint8_t *buf, uint16_t *size)
{
    if (*(uint16_t *)&buf != SYMBOL_CNT) {
        return;
    }

    if (!memcmp(openflap_ctx->config.symbol_set, buf + 2, 4 * SYMBOL_CNT)) {
        return;
    }
    memcpy(openflap_ctx->config.symbol_set, buf + 2, 4 * SYMBOL_CNT);
    openflap_ctx->store_config = true;
}
void property_character_set_get(uint8_t *buf, uint16_t *size)
{
    *size = 4 * SYMBOL_CNT; /* bytes per character to support UTF-8. */
    memcpy(buf, openflap_ctx->config.symbol_set, 4 * SYMBOL_CNT);
}

void property_offset_set(uint8_t *buf, uint16_t *size)
{
    if (openflap_ctx->config.encoder_offset == buf[0]) {
        return;
    }
    distance_update(openflap_ctx);
    openflap_ctx->config.encoder_offset = buf[0];
    openflap_ctx->store_config          = true;
}
void property_offset_get(uint8_t *buf, uint16_t *size)
{
    buf[0] = openflap_ctx->config.encoder_offset;
}

void property_character_set(uint8_t *buf, uint16_t *size)
{
    openflap_ctx->flap_setpoint = buf[0];
    distance_update(openflap_ctx);
    if (openflap_ctx->flap_distance < openflap_ctx->config.minimum_rotation) {
        openflap_ctx->extend_revolution = true;
        openflap_ctx->flap_distance += SYMBOL_CNT;
    }
}

void property_character_get(uint8_t *buf, uint16_t *size)
{
    buf[0] = flap_position_get(openflap_ctx);
}

void minimum_rotation_property_set(uint8_t *buf, uint16_t *size)
{
    if (openflap_ctx->config.minimum_rotation == buf[0]) {
        return;
    }
    openflap_ctx->config.minimum_rotation = (buf[0] < SYMBOL_CNT ? buf[0] : SYMBOL_CNT);
    openflap_ctx->store_config            = true;
}

void minimum_rotation_property_get(uint8_t *buf, uint16_t *size)
{
    buf[0] = openflap_ctx->config.minimum_rotation;
}

void color_property_set(uint8_t *buf, uint16_t *size)
{
    uint32_t foreground = (uint32_t)buf[0] << 16 | (uint32_t)buf[1] << 8 | (uint32_t)buf[2];
    uint32_t background = (uint32_t)buf[3] << 16 | (uint32_t)buf[4] << 8 | (uint32_t)buf[5];
    if (openflap_ctx->config.color.foreground == foreground && openflap_ctx->config.color.background == background) {
        return;
    }
    openflap_ctx->config.color.foreground = foreground;
    openflap_ctx->config.color.background = background;
    openflap_ctx->store_config            = true;
}

void color_property_get(uint8_t *buf, uint16_t *size)
{
    buf[0] = openflap_ctx->config.color.foreground >> 16;
    buf[1] = openflap_ctx->config.color.foreground >> 8;
    buf[2] = openflap_ctx->config.color.foreground;
    buf[3] = openflap_ctx->config.color.background >> 16;
    buf[4] = openflap_ctx->config.color.background >> 8;
    buf[5] = openflap_ctx->config.color.background;
}

void motion_property_set(uint8_t *buf, uint16_t *size)
{
    if (openflap_ctx->config.motion.min_pwm == buf[0] && openflap_ctx->config.motion.max_pwm == buf[1] &&
        openflap_ctx->config.motion.min_ramp_distance == buf[2] &&
        openflap_ctx->config.motion.max_ramp_distance == buf[3]) {
        return;
    }
    openflap_ctx->config.motion.min_pwm           = (buf[0] >= ABS_MIN_PWM ? buf[0] : ABS_MIN_PWM);
    openflap_ctx->config.motion.max_pwm           = (buf[1] <= ABS_MAX_PWM ? buf[1] : ABS_MAX_PWM);
    openflap_ctx->config.motion.min_ramp_distance = buf[2];
    openflap_ctx->config.motion.max_ramp_distance = buf[3];
    openflap_ctx->store_config                    = true;
}

void motion_property_get(uint8_t *buf, uint16_t *size)
{
    buf[0] = openflap_ctx->config.motion.min_pwm;
    buf[1] = openflap_ctx->config.motion.max_pwm;
    buf[2] = openflap_ctx->config.motion.min_ramp_distance;
    buf[3] = openflap_ctx->config.motion.max_ramp_distance;
}

void ir_threshold_property_set(uint8_t *buf, uint16_t *size)
{
    uint16_t lower = (uint16_t)buf[0] << 8 | (uint16_t)buf[1];
    uint16_t upper = (uint16_t)buf[2] << 8 | (uint16_t)buf[3];
    if (openflap_ctx->config.ir_threshold.lower == lower && openflap_ctx->config.ir_threshold.upper == upper) {
        return;
    }
    if (lower > upper) {
        return;
    }
    openflap_ctx->config.ir_threshold.lower = lower;
    openflap_ctx->config.ir_threshold.upper = upper;
    openflap_ctx->store_config              = true;
}

void ir_threshold_property_get(uint8_t *buf, uint16_t *size)
{
    buf[0] = openflap_ctx->config.ir_threshold.lower >> 8;
    buf[1] = openflap_ctx->config.ir_threshold.lower;
    buf[2] = openflap_ctx->config.ir_threshold.upper >> 8;
    buf[3] = openflap_ctx->config.ir_threshold.upper;
}

void property_handlers_init(openflap_ctx_t *ctx)
{
    openflap_ctx = ctx;

    openflap_ctx->chain_ctx.property_handler[PROPERTY_FIRMWARE_VERSION].set = NULL;
    openflap_ctx->chain_ctx.property_handler[PROPERTY_FIRMWARE_VERSION].get = property_firmware_get;

    openflap_ctx->chain_ctx.property_handler[PROPERTY_FIRMWARE_UPDATE].set = property_firmware_set;
    openflap_ctx->chain_ctx.property_handler[PROPERTY_FIRMWARE_UPDATE].get = NULL;

    openflap_ctx->chain_ctx.property_handler[PROPERTY_COMMAND].set = property_command_set;
    openflap_ctx->chain_ctx.property_handler[PROPERTY_COMMAND].get = NULL;

    openflap_ctx->chain_ctx.property_handler[PROPERTY_MODULE_INFO].set = NULL;
    openflap_ctx->chain_ctx.property_handler[PROPERTY_MODULE_INFO].get = property_module_info_get;

    openflap_ctx->chain_ctx.property_handler[PROPERTY_OFFSET].get = property_offset_get;
    openflap_ctx->chain_ctx.property_handler[PROPERTY_OFFSET].set = property_offset_set;

    openflap_ctx->chain_ctx.property_handler[PROPERTY_CHARACTER_SET].set = property_character_set_set;
    openflap_ctx->chain_ctx.property_handler[PROPERTY_CHARACTER_SET].get = property_character_set_get;

    openflap_ctx->chain_ctx.property_handler[PROPERTY_CHARACTER].set = property_character_set;
    openflap_ctx->chain_ctx.property_handler[PROPERTY_CHARACTER].get = property_character_get;

    openflap_ctx->chain_ctx.property_handler[PROPERTY_MINIMUM_ROTATION].set = minimum_rotation_property_set;
    openflap_ctx->chain_ctx.property_handler[PROPERTY_MINIMUM_ROTATION].get = minimum_rotation_property_get;

    openflap_ctx->chain_ctx.property_handler[PROPERTY_COLOR].set = color_property_set;
    openflap_ctx->chain_ctx.property_handler[PROPERTY_COLOR].get = color_property_get;

    openflap_ctx->chain_ctx.property_handler[PROPERTY_MOTION].set = motion_property_set;
    openflap_ctx->chain_ctx.property_handler[PROPERTY_MOTION].get = motion_property_get;

    openflap_ctx->chain_ctx.property_handler[PROPERTY_IR_THRESHOLD].set = ir_threshold_property_set;
    openflap_ctx->chain_ctx.property_handler[PROPERTY_IR_THRESHOLD].get = ir_threshold_property_get;
}
