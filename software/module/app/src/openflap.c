#include "openflap.h"

#define MOTOR_IDLE_TIMEOUT 500
#define COMMS_IDLE_TIMEOUT 75

uint8_t pwmDutyCycleCalc(uint8_t distance)
{
    const uint8_t min_pwm           = 40;
    const uint8_t max_pwm           = 120;
    const uint8_t min_ramp_distance = 1;  /* Go min speed when distance is below this. */
    const uint8_t max_ramp_distance = 15; /* Go max speed when distance is above this. */

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
    static uint8_t old_position = SYMBOL_CNT;
    uint8_t encoder_graycode    = 0;

    for (uint8_t i = 0; i < ENCODER_RESOLUTION; i++) {
        if (adc_data[IR_MAP[i]] > ctx->config.ir_limits[i]) {
            encoder_graycode &= ~(1 << i);
        } else if (adc_data[IR_MAP[i]] < ctx->config.ir_limits[i]) {
            encoder_graycode |= (1 << i);
        }
    }

    // Convert grey code into decimal.
    uint8_t encoder_decimal = 0;
    for (encoder_decimal = 0; encoder_graycode; encoder_graycode = encoder_graycode >> 1) {
        encoder_decimal ^= encoder_graycode;
    }
    // Reverse encoder direction.
    uint8_t new_position = (uint8_t)SYMBOL_CNT - encoder_decimal - 1;

    // Ignore erroneous reading.
    if (new_position < SYMBOL_CNT) {
        new_position = flapIndexWrapCalc(new_position + ctx->config.encoder_offset);
        // Ignore sensor backspin.
        if (flapIndexWrapCalc(new_position + 1) != old_position) {
            old_position       = new_position;
            ctx->flap_position = new_position;
        }
    }
}

uint8_t getAdcBasedRandSeed(uint32_t *adc_data)
{
    /* Use ADC noise to generate a random number. */
    uint8_t rand_seed = 0;
    for (uint8_t i = 0; i < ENCODER_RESOLUTION; i++) {
        rand_seed <<= 1;
        rand_seed ^= (adc_data[i] & 0x03);
    }
    return rand_seed;
}

void updateMotorState(openflap_ctx_t *ctx)
{
    uint8_t distance = flapIndexWrapCalc(SYMBOL_CNT + ctx->flap_setpoint - ctx->flap_position);
    if (distance > 0) {
        ctx->motor_active_timeout_tick = HAL_GetTick() + MOTOR_IDLE_TIMEOUT;
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
        ctx->comms_active_timeout_tick = HAL_GetTick() + COMMS_IDLE_TIMEOUT;
        if (!ctx->comms_active) {
            ctx->comms_active = true;
            debug_io_log_info("Comms Active\n");
            debug_io_log_disable(); // Writing to RTT may fuck up the UART RX interrupt...
        }
    } else if (ctx->comms_active && HAL_GetTick() > ctx->comms_active_timeout_tick) {
        ctx->comms_active = false;
        debug_io_log_enable();
        debug_io_log_info("Comms Idle\n");
    }
}

void setMotor(motorMode_t mode, uint8_t speed)
{
    switch (mode) {
        case MOTOR_REVERSE:
            /* Disabled because this might cause damage to the system.
             * The mechanism is designed to locks up in reverse direction. */
            // __HAL_TIM_SET_COMPARE(&motorPwmHandle, TIM_CHANNEL_1, speed);
            // HAL_GPIO_WritePin(MOTOR_A_GPIO_PORT, MOTOR_A_GPIO_PIN, GPIO_PIN_RESET);
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
