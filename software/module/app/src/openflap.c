#include "openflap.h"

#define COMMS_IDLE_TIMEOUT_MS (75)

uint8_t pwmDutyCycleCalc(const openflap_motion_config_t *cfg, uint8_t distance)
{
    if (distance <= cfg->min_ramp_distance) {
        return cfg->min_pwm;
    }

    if (distance >= cfg->max_ramp_distance) {
        return cfg->max_pwm;
    }

    return (distance - cfg->min_ramp_distance) * (cfg->max_pwm - cfg->min_pwm) /
               (cfg->max_ramp_distance - cfg->min_ramp_distance) +
           cfg->min_pwm;
}

void homingPositionDecode(openflap_ctx_t *ctx, uint32_t *adc_data)
{
    static uint32_t thresh_hold_cnt = 0;
    if (adc_data[0] > 200) {
        if (thresh_hold_cnt > 60) {
            /* White to Black transition. */
            ctx->home_found = true;
            stepper_driver_home_set(&ctx->stepper_ctx);
            debug_io_log_info("Home!\n");
        }
        thresh_hold_cnt = 0;
    } else {
        thresh_hold_cnt++;
    }
}

void updateCommsState(openflap_ctx_t *ctx)
{
    if (chain_comm_is_busy(&ctx->chain_ctx)) {
        ctx->comms_active_timeout_tick = HAL_GetTick() + COMMS_IDLE_TIMEOUT_MS;
        if (!ctx->comms_active) {
            ctx->comms_active = true;
            debug_io_log_info("Comms Active\n");
#if !CHAIN_COMM_DEBUG
            debug_io_log_set_level(LOG_LVL_WARN); /* Writing to RTT may fuck up the UART RX interrupt... */
#endif
        }
    } else if (ctx->comms_active && HAL_GetTick() > ctx->comms_active_timeout_tick) {
        ctx->comms_active = false;
#if !CHAIN_COMM_DEBUG
        debug_io_log_restore();
#endif
        debug_io_log_info("Comms Idle\n");
    }
}

int32_t wrapNumberAroundBase(int32_t number, int32_t base)
{
    return (((number % base) + base) % base);
}