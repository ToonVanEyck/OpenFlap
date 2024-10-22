#include "property_handlers.h"
#include "config.h"
#include "debug_io.h"
#include "flash.h"
#include "memory_map.h"

static openflap_ctx_t *openflap_ctx = NULL;

void firmware_property_set(uint8_t *buf)
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

void command_property_set(uint8_t *buf)
{
    switch (buf[0]) {
        case reboot_command:
            /* Reboot is handled later to allow graceful end of communication. */
            openflap_ctx->reboot = true;
            break;
        default:
            break;
    }
}

void columnEnd_property_get(uint8_t *buf)
{
    buf[0] = HAL_GPIO_ReadPin(COLEND_GPIO_PORT, COLEND_GPIO_PIN);
}

void characterMapSize_property_get(uint8_t *buf)
{
    buf[0] = SYMBOL_CNT;
}

void characterMap_property_set(uint8_t *buf)
{
    if (!memcmp(openflap_ctx->config.symbol_set, buf, 4 * SYMBOL_CNT)) {
        return;
    }
    memcpy(openflap_ctx->config.symbol_set, buf, 4 * SYMBOL_CNT);
    openflap_ctx->store_config = true;
}
void characterMap_property_get(uint8_t *buf)
{
    memcpy(buf, openflap_ctx->config.symbol_set, 4 * SYMBOL_CNT);
}

void offset_property_set(uint8_t *buf)
{
    if (openflap_ctx->config.encoder_offset == buf[0]) {
        return;
    }
    openflap_ctx->config.encoder_offset = buf[0];
    openflap_ctx->store_config          = true;
}
void offset_property_get(uint8_t *buf)
{
    buf[0] = openflap_ctx->config.encoder_offset;
}

void vtrim_property_set(uint8_t *buf)
{
    if (openflap_ctx->config.vtrim == buf[0]) {
        return;
    }
    openflap_ctx->config.vtrim = buf[0];
    openflap_ctx->store_config = true;
}
void vtrim_property_get(uint8_t *buf)
{
    buf[0] = openflap_ctx->config.vtrim;
}

void character_property_set(uint8_t *buf)
{
    openflap_ctx->flap_setpoint = buf[0];
    uint8_t distance = flapIndexWrapCalc(SYMBOL_CNT + openflap_ctx->flap_setpoint - openflap_ctx->flap_position);
    openflap_ctx->extend_revolution = (distance < openflap_ctx->config.minimum_distance);
}

void character_property_get(uint8_t *buf)
{
    buf[0] = openflap_ctx->flap_position;
}

void baseSpeed_property_set(uint8_t *buf)
{
    if (openflap_ctx->config.base_speed == buf[0]) {
        return;
    }
    openflap_ctx->config.base_speed = buf[0];
    openflap_ctx->store_config      = true;
}

void baseSpeed_property_get(uint8_t *buf)
{
    buf[0] = openflap_ctx->config.base_speed;
}

void property_handlers_init(openflap_ctx_t *ctx)
{
    openflap_ctx = ctx;

    openflap_ctx->chain_ctx.property_handler[firmware_property].set = firmware_property_set;
    openflap_ctx->chain_ctx.property_handler[firmware_property].get = NULL;

    openflap_ctx->chain_ctx.property_handler[command_property].set = command_property_set;
    openflap_ctx->chain_ctx.property_handler[command_property].get = NULL;

    openflap_ctx->chain_ctx.property_handler[columnEnd_property].set = NULL;
    openflap_ctx->chain_ctx.property_handler[columnEnd_property].get = columnEnd_property_get;

    openflap_ctx->chain_ctx.property_handler[characterMapSize_property].set = NULL;
    openflap_ctx->chain_ctx.property_handler[characterMapSize_property].get = characterMapSize_property_get;

    openflap_ctx->chain_ctx.property_handler[offset_property].get = offset_property_get;
    openflap_ctx->chain_ctx.property_handler[offset_property].set = offset_property_set;

    openflap_ctx->chain_ctx.property_handler[vtrim_property].set = vtrim_property_set;
    openflap_ctx->chain_ctx.property_handler[vtrim_property].get = vtrim_property_get;

    openflap_ctx->chain_ctx.property_handler[characterMap_property].set = characterMap_property_set;
    openflap_ctx->chain_ctx.property_handler[characterMap_property].get = characterMap_property_get;

    openflap_ctx->chain_ctx.property_handler[character_property].set = character_property_set;
    openflap_ctx->chain_ctx.property_handler[character_property].get = character_property_get;

    openflap_ctx->chain_ctx.property_handler[baseSpeed_property].set = baseSpeed_property_set;
    openflap_ctx->chain_ctx.property_handler[baseSpeed_property].get = baseSpeed_property_get;
}
