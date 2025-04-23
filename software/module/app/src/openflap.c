#include "openflap.h"

#define MOTOR_IDLE_TIMEOUT_MS (500)
#define COMMS_IDLE_TIMEOUT_MS (75)

/** The motor will reverse direction for this duration after reaching the desired flap. */
#define MOTOR_BACKSPIN_DURATION_MS (50)
/** The motor will revers direction with this pwm value after reaching the desired flap. */
#define MOTOR_BACKSPIN_PWM (125)

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

void encoderPositionUpdate(openflap_ctx_t *ctx, uint32_t *adc_data)
{
    static const uint8_t QEM[16]   = {0, -1, 1, 2, 1, 0, 2, -1, -1, 2, 0, 1, 2, 1, -1, 0};
    static const uint8_t IR_MAP[2] = {ENCODER_CHANNEL_A, ENCODER_CHANNEL_B};

    /* Prevent decrementing the flap position, instead wind up the backspin prevention counter. */
    static int8_t backspin_prevention = -SYMBOL_CNT;
    /* Prevent jumping back from 1 to zero. */
    static bool zero_lockout = false;
    /* Encoder increment/decrement is determined based on the old and new pattern. */
    static uint8_t old_pattern = 0x00;
    uint8_t new_pattern        = old_pattern;

    /* Digitize the ADC signals using the configured IR thresholds. */
    bool zero = adc_data[ENCODER_CHANNEL_Z] < ctx->config.ir_threshold.lower;
    for (uint8_t i = 0; i < 2; i++) {
        if (adc_data[IR_MAP[i]] > ctx->config.ir_threshold.upper) {
            new_pattern &= ~(1 << i);
        } else if (adc_data[IR_MAP[i]] < ctx->config.ir_threshold.lower) {
            new_pattern |= (1 << i);
        }
    }

    int8_t qem = QEM[(old_pattern << 2) | new_pattern];

    /* Update the old_pattern. */
    old_pattern = new_pattern;

    if (qem == 2) {
        debug_io_log_error("Invalid QEM value: %d\n", qem);
        return;
    }

    /* Check if we have a zero or full pattern. */
    if (zero & !zero_lockout) {
        encoderZero(ctx);
        backspin_prevention = 0;
        zero_lockout        = true;
    } else if (backspin_prevention < 0 || qem != 1) {
        backspin_prevention += qem; /* Only increment the flap position once the backspin prevention reaches 0. */
    } else {
        encoderIncrement(ctx);
        if (ctx->flap_position > 5) { /* Make sure we are well past 1 so wid dont flipper between 1 and 0. */
            zero_lockout = false;
        }
    }
}

void distanceUpdate(openflap_ctx_t *ctx)
{
    uint8_t distance = flapIndexWrapCalc(SYMBOL_CNT + ctx->flap_setpoint - flapPostionGet(ctx));
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

void updateMotorState(openflap_ctx_t *ctx)
{
    if (ctx->flap_distance > 0) {
        ctx->motor_active_timeout_tick = HAL_GetTick() + MOTOR_IDLE_TIMEOUT_MS;
        if (!ctx->motor_active) {
            ctx->motor_active = true;
            debug_io_log_info("Motor Active\n");
        }
    } else if (ctx->motor_active && HAL_GetTick() > ctx->motor_active_timeout_tick) {
        ctx->motor_active = false;
        debug_io_log_info("Motor Idle\n");
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

void setMotorFromDistance(openflap_ctx_t *ctx)
{
    if (ctx->flap_distance > 0) {
        ctx->motor_backspin_timeout_tick = 0;
        setMotor(MOTOR_FORWARD_WITH_BREAK, pwmDutyCycleCalc(&ctx->config.motion, ctx->flap_distance));
    } else {
        if (ctx->motor_backspin_timeout_tick == 0) {
            ctx->motor_backspin_timeout_tick = HAL_GetTick() + MOTOR_BACKSPIN_DURATION_MS;
            setMotor(MOTOR_REVERSE, MOTOR_BACKSPIN_PWM);
        }
        if (HAL_GetTick() >= ctx->motor_backspin_timeout_tick) {
            motorBrake();
        }
    }
}

void setMotor(motorMode_t mode, uint8_t speed)
{
    switch (mode) {
        case MOTOR_REVERSE:
            /* Use with caution, this might cause damage to the system.
             * The mechanism is designed to locks up in reverse direction. */
            __HAL_TIM_SET_COMPARE(&motorPwmHandle, TIM_CHANNEL_1, speed);
            __HAL_TIM_SET_COMPARE(&motorPwmHandle, TIM_CHANNEL_2, 0x00);
            break;
        case MOTOR_FORWARD:
            __HAL_TIM_SET_COMPARE(&motorPwmHandle, TIM_CHANNEL_1, 0x00);
            __HAL_TIM_SET_COMPARE(&motorPwmHandle, TIM_CHANNEL_2, speed);
            break;
        case MOTOR_FORWARD_WITH_BREAK:
            /* This PWM scheme actively breaks (HH) for one part of the period, rotates forward (HL) for an equal part
             * of the period and then remains idle (LL) for the rest of the period. */
            __HAL_TIM_SET_COMPARE(&motorPwmHandle, TIM_CHANNEL_1, 0xff - speed);
            __HAL_TIM_SET_COMPARE(&motorPwmHandle, TIM_CHANNEL_2, 0xff);
            break;
        case MOTOR_BRAKE:
            /* Set MOTOR A & B high, braking the motor. */
            __HAL_TIM_SET_COMPARE(&motorPwmHandle, TIM_CHANNEL_1, 0xff);
            __HAL_TIM_SET_COMPARE(&motorPwmHandle, TIM_CHANNEL_2, 0xff);
            break;
        case MOTOR_IDLE:
        default:
            /* Set MOTOR A & B low, let the motor freewheel. */
            __HAL_TIM_SET_COMPARE(&motorPwmHandle, TIM_CHANNEL_1, 0x00);
            __HAL_TIM_SET_COMPARE(&motorPwmHandle, TIM_CHANNEL_2, 0x00);
            break;
    }
}

void encoderIncrement(openflap_ctx_t *ctx)
{
    ctx->flap_position = flapIndexWrapCalc(ctx->flap_position + 1);
    distanceUpdate(ctx);
}

void encoderZero(openflap_ctx_t *ctx)
{
    ctx->flap_position = 0;
    distanceUpdate(ctx);
}

uint8_t flapPostionGet(openflap_ctx_t *ctx)
{
    return flapIndexWrapCalc(ctx->flap_position + ctx->config.encoder_offset);
}