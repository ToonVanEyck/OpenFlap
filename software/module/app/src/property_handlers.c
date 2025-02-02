#include "property_handlers.h"
#include "config.h"
#include "debug_io.h"
#include "flash.h"
#include "memory_map.h"

static openflap_ctx_t *openflap_ctx = NULL; /* Set during initialization. */

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
    strncpy((char *)buf, GIT_VERSION, CHAIN_COM_MAX_LEN - 1);
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
    module_info.column_end             = HAL_GPIO_ReadPin(COLEND_GPIO_PORT, COLEND_GPIO_PIN);
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

void property_calibration_set(uint8_t *buf, uint16_t *size)
{
    if (openflap_ctx->config.encoder_offset == buf[0]) {
        return;
    }
    openflap_ctx->config.encoder_offset = buf[0];
    openflap_ctx->store_config          = true;
}
void property_calibration_get(uint8_t *buf, uint16_t *size)
{
    buf[0] = openflap_ctx->config.encoder_offset;
}

void property_character_set(uint8_t *buf, uint16_t *size)
{
    openflap_ctx->flap_setpoint = buf[0];
    distanceUpdate(openflap_ctx);
    if (openflap_ctx->flap_distance < openflap_ctx->config.minimum_distance) {
        openflap_ctx->extend_revolution = true;
        openflap_ctx->flap_distance += SYMBOL_CNT;
    }
}

void property_character_get(uint8_t *buf, uint16_t *size)
{
    buf[0] = openflap_ctx->flap_position;
}

void baseSpeed_property_set(uint8_t *buf, uint16_t *size)
{
    if (openflap_ctx->config.base_speed == buf[0]) {
        return;
    }
    openflap_ctx->config.base_speed = buf[0];
    openflap_ctx->store_config      = true;
}

void baseSpeed_property_get(uint8_t *buf, uint16_t *size)
{
    buf[0] = openflap_ctx->config.base_speed;
}

void property_handlers_init(openflap_ctx_t *ctx)
{
    openflap_ctx = ctx;

    openflap_ctx->chain_ctx.property_handler[PROPERTY_FIRMWARE].set = property_firmware_set;
    openflap_ctx->chain_ctx.property_handler[PROPERTY_FIRMWARE].get = property_firmware_get;

    openflap_ctx->chain_ctx.property_handler[PROPERTY_COMMAND].set = property_command_set;
    openflap_ctx->chain_ctx.property_handler[PROPERTY_COMMAND].get = NULL;

    openflap_ctx->chain_ctx.property_handler[PROPERTY_MODULE_INFO].set = NULL;
    openflap_ctx->chain_ctx.property_handler[PROPERTY_MODULE_INFO].get = property_module_info_get;

    openflap_ctx->chain_ctx.property_handler[PROPERTY_CALIBRATION].get = property_calibration_get;
    openflap_ctx->chain_ctx.property_handler[PROPERTY_CALIBRATION].set = property_calibration_set;

    openflap_ctx->chain_ctx.property_handler[PROPERTY_CHARACTER_SET].set = property_character_set_set;
    openflap_ctx->chain_ctx.property_handler[PROPERTY_CHARACTER_SET].get = property_character_set_get;

    openflap_ctx->chain_ctx.property_handler[PROPERTY_CHARACTER].set = property_character_set;
    openflap_ctx->chain_ctx.property_handler[PROPERTY_CHARACTER].get = property_character_get;
}
