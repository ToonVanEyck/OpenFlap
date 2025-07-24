#include "openflap.h"

#define MOTOR_IDLE_TIMEOUT_MS (500)
#define COMMS_IDLE_TIMEOUT_MS (75)

/** The motor will reverse direction for this duration after reaching the desired flap. */
#define MOTOR_BACKSPIN_DURATION_MS (250)
/** The motor will revers direction with this pwm value after reaching the desired flap. */
#define MOTOR_BACKSPIN_PWM (300)

uint8_t pwm_duty_cycle_calc(const openflap_motion_config_t *cfg, uint8_t distance)
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

void encoder_position_update(openflap_ctx_t *ctx, const encoder_states_t *encoder_reading)
{
    static const uint8_t QEM[16] = {0, -1, 1, 2, 1, 0, 2, -1, -1, 2, 0, 1, 2, 1, -1, 0};

    /* Prevent decrementing the flap position, instead wind up the backspin prevention counter. */
    static int8_t backspin_prevention = -SYMBOL_CNT;
    /* Prevent jumping back from 1 to zero. */
    static bool zero_lockout = false;
    /* Encoder increment/decrement is determined based on the old and new pattern. */
    static uint8_t old_pattern = 0x00;

    /* Digitize the ADC signals using the configured IR thresholds. */
    bool zero           = encoder_reading->enc_z;
    uint8_t new_pattern = (encoder_reading->enc_a << 1) | (encoder_reading->enc_b << 0);

    int8_t qem = QEM[(old_pattern << 2) | new_pattern];

    /* Update the old_pattern. */
    old_pattern = new_pattern;

    if (qem == 2) {
        debug_io_log_error("Invalid QEM value: %d\n", qem);
        return;
    }

    /* Check if we have a zero or full pattern. */
    if (zero & !zero_lockout) {
        encoder_zero(ctx);
        backspin_prevention = 0;
        zero_lockout        = true;
    } else if (backspin_prevention < 0 || qem != 1) {
        backspin_prevention += qem; /* Only increment the flap position once the backspin prevention reaches 0. */
    } else {
        encoder_increment(ctx);
        if (ctx->flap_position > 5) { /* Make sure we are well past 1 so wid dont flipper between 1 and 0. */
            zero_lockout = false;
        }
    }
}

void distance_update(openflap_ctx_t *ctx)
{
    uint8_t distance = flapIndex_wrap_calc(SYMBOL_CNT + ctx->flap_setpoint - flap_position_get(ctx));
    /* Check if a short rotation needs to be extended. */
    if (ctx->extend_revolution) {
        if (distance < ctx->config.minimum_rotation) {
            distance += SYMBOL_CNT;
        } else {
            ctx->extend_revolution = false;
        }
    }
    ctx->flap_distance = distance;
}

void motor_state_update(openflap_ctx_t *ctx)
{
    if (ctx->flap_distance > 0) {
        ctx->motor_active_timeout_tick = get_tick_count() + MOTOR_IDLE_TIMEOUT_MS;
        if (!ctx->motor_active) {
            ctx->motor_active = true;
            debug_io_log_info("Motor Active\n");
        }
    } else if (ctx->motor_active && get_tick_count() > ctx->motor_active_timeout_tick) {
        ctx->motor_active = false;
        debug_io_log_info("Motor Idle\n");
    }
}

void comms_state_update(openflap_ctx_t *ctx)
{
    if (chain_comm_is_busy(&ctx->chain_ctx)) {
        ctx->comms_active_timeout_tick = get_tick_count() + COMMS_IDLE_TIMEOUT_MS;
        if (!ctx->comms_active) {
            ctx->comms_active = true;
            debug_io_log_info("Comms Active\n");
#if !CHAIN_COMM_DEBUG
            debug_io_log_set_level(LOG_LVL_WARN); /* Writing to RTT may fuck up the UART RX interrupt... */
#endif
        }
    } else if (ctx->comms_active && get_tick_count() > ctx->comms_active_timeout_tick) {
        ctx->comms_active = false;
#if !CHAIN_COMM_DEBUG
        debug_io_log_restore();
#endif
        debug_io_log_info("Comms Idle\n");
    }
}

void from_distance_motor_set(openflap_ctx_t *ctx, motor_ctx_t *motor_ctx)
{
    if (ctx->flap_distance > 5) {
        pid_i_lim_update(&ctx->pid_ctx, 0, 0);
    } else {
        pid_i_lim_update(&ctx->pid_ctx, 0, 105000);
    }

    int16_t pid_output = pid_compute(&ctx->pid_ctx, ctx->flap_distance);

    if (ctx->flap_distance == 0) {
        ctx->pid_ctx.integral = 0;
        pid_output            = 0;
    }

    /* Set the motor speed based on the PID output. */
    if (pid_output < 0) {
        ctx->motor_backspin_timeout_tick = 0;
        motor_ctx->mode                  = MOTOR_REVERSE;
        motor_ctx->speed                 = -pid_output;
    } else if (pid_output > 0) {
        ctx->motor_backspin_timeout_tick = 0;
        motor_ctx->mode                  = MOTOR_FORWARD;
        motor_ctx->speed                 = pid_output;
    } else {
        if (ctx->motor_backspin_timeout_tick == 0) {
            ctx->motor_backspin_timeout_tick = get_tick_count() + MOTOR_BACKSPIN_DURATION_MS;
            motor_ctx->mode                  = MOTOR_REVERSE;
            motor_ctx->speed                 = MOTOR_BACKSPIN_PWM;
        }
        if (get_tick_count() >= ctx->motor_backspin_timeout_tick) {
            motor_ctx->mode  = MOTOR_BRAKE; // Set to brake if speed is zero
            motor_ctx->speed = 0;
        }
    }
    motor_control(motor_ctx);
}

void encoder_increment(openflap_ctx_t *ctx)
{
    ctx->flap_position = flapIndex_wrap_calc(ctx->flap_position + 1);
    distance_update(ctx);
}

void encoder_zero(openflap_ctx_t *ctx)
{
    ctx->flap_position = 0;
    distance_update(ctx);
}

uint8_t flap_position_get(openflap_ctx_t *ctx)
{
    return flapIndex_wrap_calc(ctx->flap_position + ctx->config.encoder_offset);
}