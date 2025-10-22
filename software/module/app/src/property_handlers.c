#include "property_handlers.h"
#include "flash.h"
#include "memory_map.h"
#include "openflap.h"

#include <stdint.h>
#include <string.h>

static of_ctx_t *of_ctx = NULL; /* Set during initialization. */

extern uint32_t checksum;

bool property_firmware_set(void *userdata, uint16_t node_idx, uint8_t *buf, size_t *size)
{
    uint32_t addr_base   = (uint32_t)(APP_START_PTR + (NEW_APP * APP_SIZE / 4));
    uint32_t addr_offset = ((uint32_t)buf[0] << 8 | (uint32_t)buf[1]) * FLASH_PAGE_SIZE;
    uint32_t addr        = addr_base + addr_offset;
    flash_write(addr, (buf + 2), FLASH_PAGE_SIZE);
    return true;
}

bool property_firmware_get(void *userdata, uint16_t node_idx, uint8_t *buf, size_t *size)
{
    *size = strlen(GIT_VERSION);
    strncpy((char *)buf, GIT_VERSION, CC_PAYLOAD_SIZE_MAX); // Reserve 4 bytes for checksum
    // append checksum bytes
    buf[(*size)++] = (checksum >> 24) & 0xFF;
    buf[(*size)++] = (checksum >> 16) & 0xFF;
    buf[(*size)++] = (checksum >> 8) & 0xFF;
    buf[(*size)++] = checksum & 0xFF;
    return true;
}

bool property_command_set(void *userdata, uint16_t node_idx, uint8_t *buf, size_t *size)
{
    switch (buf[0]) {
        case CMD_REBOOT:
            /* Reboot is handled later to allow graceful end of communication. */
            of_ctx->reboot = true;
            break;
        case CMD_MOTOR_UNLOCK:
            of_ctx->motor_control_override = false; /* Enable the motor control. */
        default:
            break;
    }
    return true;
}

bool property_module_info_get(void *userdata, uint16_t node_idx, uint8_t *buf, size_t *size)
{
    *size          = 0;
    buf[(*size)++] = of_ctx->column_end;
    return true;
}

bool property_character_set_set(void *userdata, uint16_t node_idx, uint8_t *buf, size_t *size)
{
    if (*(uint16_t *)&buf != SYMBOL_CNT) {
        return false;
    }

    if (!memcmp(of_ctx->of_config.symbol_set, buf + 2, 4 * SYMBOL_CNT)) {
        return false;
    }
    memcpy(of_ctx->of_config.symbol_set, buf + 2, 4 * SYMBOL_CNT);
    of_ctx->store_config = true;
    return true;
}
bool property_character_set_get(void *userdata, uint16_t node_idx, uint8_t *buf, size_t *size)
{
    *size = 4 * SYMBOL_CNT; /* bytes per character to support UTF-8. */
    memcpy(buf, of_ctx->of_config.symbol_set, 4 * SYMBOL_CNT);
    return true;
}

bool property_offset_set(void *userdata, uint16_t node_idx, uint8_t *buf, size_t *size)
{
    if (of_ctx->of_config.encoder_offset == buf[0]) {
        return false;
    }
    int16_t offset_delta             = (int16_t)of_ctx->of_config.encoder_offset - (int16_t)buf[0];
    of_ctx->of_config.encoder_offset = buf[0];
    of_ctx->flap_position            = flapIndex_wrap_calc((int16_t)(of_ctx->flap_position) + offset_delta);
    distance_update(of_ctx);
    of_ctx->store_config = true;
    return true;
}
bool property_offset_get(void *userdata, uint16_t node_idx, uint8_t *buf, size_t *size)
{
    *size          = 0;
    buf[(*size)++] = of_ctx->of_config.encoder_offset;
    return true;
}

bool property_character_set(void *userdata, uint16_t node_idx, uint8_t *buf, size_t *size)
{
    of_ctx->flap_setpoint = buf[0] * ENCODER_PULSES_PER_SYMBOL;
    distance_update(of_ctx);
    if (of_ctx->flap_distance < of_ctx->of_config.minimum_rotation * ENCODER_PULSES_PER_SYMBOL) {
        of_ctx->extend_revolution = true;
        of_ctx->flap_distance += ENCODER_PULSES_PER_REVOLUTION;
    }
    return true;
}

bool property_character_get(void *userdata, uint16_t node_idx, uint8_t *buf, size_t *size)
{
    *size          = 0;
    buf[(*size)++] = of_ctx->flap_position / ENCODER_PULSES_PER_SYMBOL;
    return true;
}

bool minimum_rotation_property_set(void *userdata, uint16_t node_idx, uint8_t *buf, size_t *size)
{
    if (of_ctx->of_config.minimum_rotation == buf[0]) {
        return false;
    }
    of_ctx->of_config.minimum_rotation = (buf[0] < SYMBOL_CNT ? buf[0] : SYMBOL_CNT);
    of_ctx->store_config               = true;
    return true;
}

bool minimum_rotation_property_get(void *userdata, uint16_t node_idx, uint8_t *buf, size_t *size)
{
    *size          = 0;
    buf[(*size)++] = of_ctx->of_config.minimum_rotation;
    return true;
}

bool color_property_set(void *userdata, uint16_t node_idx, uint8_t *buf, size_t *size)
{
    uint32_t foreground = (uint32_t)buf[0] << 16 | (uint32_t)buf[1] << 8 | (uint32_t)buf[2];
    uint32_t background = (uint32_t)buf[3] << 16 | (uint32_t)buf[4] << 8 | (uint32_t)buf[5];
    if (of_ctx->of_config.color.foreground == foreground && of_ctx->of_config.color.background == background) {
        return false;
    }
    of_ctx->of_config.color.foreground = foreground;
    of_ctx->of_config.color.background = background;
    of_ctx->store_config               = true;
    return true;
}

bool color_property_get(void *userdata, uint16_t node_idx, uint8_t *buf, size_t *size)
{
    *size          = 0;
    buf[(*size)++] = of_ctx->of_config.color.foreground >> 16;
    buf[(*size)++] = of_ctx->of_config.color.foreground >> 8;
    buf[(*size)++] = of_ctx->of_config.color.foreground;
    buf[(*size)++] = of_ctx->of_config.color.background >> 16;
    buf[(*size)++] = of_ctx->of_config.color.background >> 8;
    buf[(*size)++] = of_ctx->of_config.color.background;
    return true;
}

bool motion_property_set(void *userdata, uint16_t node_idx, uint8_t *buf, size_t *size)
{
    if (of_ctx->of_config.motion.min_pwm == buf[0] && of_ctx->of_config.motion.max_pwm == buf[1] &&
        of_ctx->of_config.motion.min_ramp_distance == buf[2] && of_ctx->of_config.motion.max_ramp_distance == buf[3]) {
        return false;
    }
    of_ctx->of_config.motion.min_pwm           = (buf[0] >= 35 ? buf[0] : 35);
    of_ctx->of_config.motion.max_pwm           = (buf[1] <= 200 ? buf[1] : 200);
    of_ctx->of_config.motion.min_ramp_distance = buf[2];
    of_ctx->of_config.motion.max_ramp_distance = buf[3];
    of_ctx->store_config                       = true;
    return true;
}

bool motion_property_get(void *userdata, uint16_t node_idx, uint8_t *buf, size_t *size)
{
    *size          = 0;
    buf[(*size)++] = of_ctx->of_config.motion.min_pwm;
    buf[(*size)++] = of_ctx->of_config.motion.max_pwm;
    buf[(*size)++] = of_ctx->of_config.motion.min_ramp_distance;
    buf[(*size)++] = of_ctx->of_config.motion.max_ramp_distance;
    return true;
}

bool ir_threshold_property_set(void *userdata, uint16_t node_idx, uint8_t *buf, size_t *size)
{
    uint16_t lower = (uint16_t)buf[0] << 8 | (uint16_t)buf[1];
    uint16_t upper = (uint16_t)buf[2] << 8 | (uint16_t)buf[3];
    if (of_ctx->of_config.ir_threshold.lower == lower && of_ctx->of_config.ir_threshold.upper == upper) {
        return false;
    }
    if (lower > upper) {
        return false;
    }
    of_ctx->of_config.ir_threshold.lower = lower;
    of_ctx->of_config.ir_threshold.upper = upper;
    of_ctx->store_config                 = true;
    return true;
}

bool ir_threshold_property_get(void *userdata, uint16_t node_idx, uint8_t *buf, size_t *size)
{
    *size          = 0;
    buf[(*size)++] = of_ctx->of_config.ir_threshold.lower >> 8;
    buf[(*size)++] = of_ctx->of_config.ir_threshold.lower;
    buf[(*size)++] = of_ctx->of_config.ir_threshold.upper >> 8;
    buf[(*size)++] = of_ctx->of_config.ir_threshold.upper;
    return true;
}

void property_handlers_init(of_ctx_t *ctx)
{
    of_ctx = ctx;

    cc_prop_list[OF_CC_PROP_FIRMWARE_VERSION].handler.set = NULL;
    cc_prop_list[OF_CC_PROP_FIRMWARE_VERSION].handler.get = property_firmware_get;

    cc_prop_list[OF_CC_PROP_FIRMWARE_UPDATE].handler.set = property_firmware_set;
    cc_prop_list[OF_CC_PROP_FIRMWARE_UPDATE].handler.get = NULL;

    cc_prop_list[OF_CC_PROP_COMMAND].handler.set = property_command_set;
    cc_prop_list[OF_CC_PROP_COMMAND].handler.get = NULL;

    cc_prop_list[OF_CC_PROP_MODULE_INFO].handler.set = NULL;
    cc_prop_list[OF_CC_PROP_MODULE_INFO].handler.get = property_module_info_get;

    cc_prop_list[OF_CC_PROP_OFFSET].handler.get = property_offset_get;
    cc_prop_list[OF_CC_PROP_OFFSET].handler.set = property_offset_set;

    cc_prop_list[OF_CC_PROP_CHARACTER_SET].handler.set = property_character_set_set;
    cc_prop_list[OF_CC_PROP_CHARACTER_SET].handler.get = property_character_set_get;

    cc_prop_list[OF_CC_PROP_CHARACTER].handler.set = property_character_set;
    cc_prop_list[OF_CC_PROP_CHARACTER].handler.get = property_character_get;

    cc_prop_list[OF_CC_PROP_MINIMUM_ROTATION].handler.set = minimum_rotation_property_set;
    cc_prop_list[OF_CC_PROP_MINIMUM_ROTATION].handler.get = minimum_rotation_property_get;

    cc_prop_list[OF_CC_PROP_COLOR].handler.set = color_property_set;
    cc_prop_list[OF_CC_PROP_COLOR].handler.get = color_property_get;

    cc_prop_list[OF_CC_PROP_MOTION].handler.set = motion_property_set;
    cc_prop_list[OF_CC_PROP_MOTION].handler.get = motion_property_get;

    cc_prop_list[OF_CC_PROP_IR_THRESHOLD].handler.set = ir_threshold_property_set;
    cc_prop_list[OF_CC_PROP_IR_THRESHOLD].handler.get = ir_threshold_property_get;
}
