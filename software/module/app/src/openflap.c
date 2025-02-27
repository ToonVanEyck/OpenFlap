#include "openflap.h"

#define MOTOR_IDLE_TIMEOUT_MS (500)
#define COMMS_IDLE_TIMEOUT_MS (75)

/** The motor will reverse direction for this duration after reaching the desired flap. */
#define MOTOR_BACKSPIN_DURATION_MS (0)
/** The motor will revers direction with this pwm value after reaching the desired flap. */
#define MOTOR_BACKSPIN_PWM (15)

uint8_t pwmDutyCycleCalc(uint8_t distance)
{
    const uint8_t min_pwm           = 50;
    const uint8_t max_pwm           = 160; /* 110 = +/- 0.5 rps = 30 rpm */
    const uint8_t min_ramp_distance = 1;   /* Go min speed when distance is below this. */
    const uint8_t max_ramp_distance = 8;   /* Go max speed when distance is above this. */

    if (distance <= min_ramp_distance) {
        return min_pwm;
    }

    if (distance >= max_ramp_distance) {
        return max_pwm;
    }

    return (distance - min_ramp_distance) * (max_pwm - min_pwm) / (max_ramp_distance - min_ramp_distance) + min_pwm;
}

void encoderPositionUpdate(openflap_ctx_t *ctx, uint32_t *adc_data)
{
    static const uint8_t QEM[16]   = {0, -1, 1, 2, 1, 0, 2, -1, -1, 2, 0, 1, 2, 1, -1, 0};
    static const uint8_t IR_MAP[2] = {ENCODER_CHANNEL_A, ENCODER_CHANNEL_B};

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

    if (qem == 0) {
        return;
    }

    if (qem == 2) {
        debug_io_log_error("Invalid QEM value: %d\n", qem);
        return;
    }

    /* Check if we have a zero or full pattern. */
    if (zero) {
        ctx->flap_position = flapIndexWrapCalc(ctx->config.encoder_offset);
    } else {
        ctx->flap_position = flapIndexWrapCalc(ctx->flap_position + SYMBOL_CNT + qem);
    }
    distanceUpdate(ctx);
}

void distanceUpdate(openflap_ctx_t *ctx)
{
    uint8_t distance = flapIndexWrapCalc(SYMBOL_CNT + ctx->flap_setpoint - ctx->flap_position);
    /* Check if a short rotation needs to be extended. */
    if (ctx->extend_revolution) {
        if (distance < ctx->config.minimum_distance) {
            distance += SYMBOL_CNT;
        } else {
            ctx->extend_revolution = false;
        }
    }
    ctx->flap_distance = distance;
}

void updateMotorState(openflap_ctx_t *ctx)
{
    uint8_t distance = flapIndexWrapCalc(SYMBOL_CNT + ctx->flap_setpoint - ctx->flap_position);
    if (distance > 0) {
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
            debug_io_log_disable(); /* Writing to RTT may fuck up the UART RX interrupt... */
#endif
        }
    } else if (ctx->comms_active && HAL_GetTick() > ctx->comms_active_timeout_tick) {
        ctx->comms_active = false;
#if !CHAIN_COMM_DEBUG
        debug_io_log_enable();
#endif
        debug_io_log_info("Comms Idle\n");
    }
}

void setMotorFromDistance(openflap_ctx_t *ctx)
{
    if (ctx->flap_distance > 0) {
        motorForward(pwmDutyCycleCalc(ctx->flap_distance));
        ctx->motor_backspin_timeout_tick = 0;
    } else {
        if (ctx->motor_backspin_timeout_tick == 0) {
            ctx->motor_backspin_timeout_tick = HAL_GetTick() + MOTOR_BACKSPIN_DURATION_MS;
            motorReverse(MOTOR_BACKSPIN_PWM);
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
            HAL_GPIO_WritePin(MOTOR_A_GPIO_PORT, MOTOR_A_GPIO_PIN, GPIO_PIN_RESET);
            break;
        case MOTOR_FORWARD:
            speed = 0xff - speed; /* Invert Duty-Cycle. */
            __HAL_TIM_SET_COMPARE(&motorPwmHandle, TIM_CHANNEL_1, speed);
            HAL_GPIO_WritePin(MOTOR_A_GPIO_PORT, MOTOR_A_GPIO_PIN, GPIO_PIN_SET);
            break;
        case MOTOR_BRAKE:
            /* Set MOTOR A & B high, braking the motor. */
            __HAL_TIM_SET_COMPARE(&motorPwmHandle, TIM_CHANNEL_1, 0xff);
            HAL_GPIO_WritePin(MOTOR_A_GPIO_PORT, MOTOR_A_GPIO_PIN, GPIO_PIN_SET);
            break;
        case MOTOR_IDLE:
        default:
            /* Set MOTOR A & B low, let the motor freewheel. */
            __HAL_TIM_SET_COMPARE(&motorPwmHandle, TIM_CHANNEL_1, 0x00);
            HAL_GPIO_WritePin(MOTOR_A_GPIO_PORT, MOTOR_A_GPIO_PIN, GPIO_PIN_RESET);
            break;
    }
}
