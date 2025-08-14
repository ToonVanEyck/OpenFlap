#include "openflap.h"
#include "debug_io.h"

#define MOTOR_IDLE_TIMEOUT_MS (500)
#define COMMS_IDLE_TIMEOUT_MS (75)

/** The motor will reverse direction for this duration after reaching the desired flap. */
#define MOTOR_BACKSPIN_DURATION_TICKS (25)
/** The motor will revers direction with this pwm value after reaching the desired flap. */
#define MOTOR_BACKSPIN_PWM (300)

#define ENCODER_SENSOR_CALIBRATION_DURATION_TICKS (5000)
#define ENCODER_SENSOR_CALIBRATION_MOTOR_PWM      (200)

typedef struct {
    uint8_t increment_pattern;
    uint8_t decrement_pattern;
} encoder_position_comparison_t;

/* Debug IO scope struct */
typedef struct {
    uint32_t setpoint;
    uint32_t actual;
    int32_t p;
    int32_t i;
    int32_t d;
    int32_t pid_out;
    int32_t feed_forward;
    int32_t cl_out;
    // uint32_t pwm;
    // uint32_t avg_speed;
} debug_io_scope_t;

//----------------------------------------------------------------------------------------------------------------------

void of_encoder_values_update(of_ctx_t *ctx)
{
    /* Get the new values from the adc. */
    of_hal_encoder_values_get(ctx->encoder.analog);

    /* Normalize values. */
    if (ctx->encoder_sensor_calibration_ticks == 0) {
        for (uint8_t i = 0; i < ENCODER_CHANNEL_COUNT; i++) {
            ctx->encoder.normalized[i] = 1000 * ((int32_t)ctx->encoder.analog[i] - ctx->of_config.enc_cal[i].min) /
                                         (ctx->of_config.enc_cal[i].max - ctx->of_config.enc_cal[i].min);
            // if (ctx->encoder.normalized[i] > 1000) {
            //     ctx->encoder.normalized[i] = 1000;
            // } else if (ctx->encoder.normalized[i] < 0) {
            //     ctx->encoder.normalized[i] = 0;
            // }
        }
    }

    /* Digitize the analog values. */
    for (uint8_t i = 0; i < ENCODER_CHANNEL_COUNT; i++) {
        if (ctx->encoder.normalized[i] < ctx->of_config.ir_threshold.lower) {
            ctx->encoder.digital[i] = true;
        } else if (ctx->encoder.normalized[i] > ctx->of_config.ir_threshold.upper) {
            ctx->encoder.digital[i] = false;
        }
    }
}

//----------------------------------------------------------------------------------------------------------------------

void of_encoder_position_update(of_ctx_t *ctx)
{
    /* Table with valid encoder patterns, invalid patterns are 0. */
    static const int8_t enc_pattern_index[16] = {
        [0b0000] = 1, [0b0001] = 2, [0b0011] = 3, [0b0111] = 4, [0b1111] = 5, [0b1110] = 6, [0b1100] = 7, [0b1000] = 8,
    };

    /* Prevent decrementing the flap position, instead wind up the backspin prevention counter. */
    static int16_t backspin_prevention = 0; //-ENCODER_PULSES_PER_REVOLUTION;
    /* Prevent jumping back from 1 to zero. */
    static bool zero_lockout = false;
    /* Encoder increment/decrement is determined based on the old and new pattern. */
    static uint8_t old_pattern = 0x00;

    /* Digitize the ADC signals using the configured IR thresholds. */
    uint8_t new_pattern = (ctx->encoder.digital[ENC_CH_A] << 0) | (ctx->encoder.digital[ENC_CH_B] << 1) |
                          (ctx->encoder.digital[ENC_CH_C] << 2) | (ctx->encoder.digital[ENC_CH_D] << 3);

    /* Pattern is invalid or didn't change. */
    if (enc_pattern_index[new_pattern] == 0 || enc_pattern_index[old_pattern] == enc_pattern_index[new_pattern]) {
        return;
    }

    int8_t pattern_diff = enc_pattern_index[old_pattern] - enc_pattern_index[new_pattern];
    pattern_diff += (pattern_diff < -4) ? 8 : 0;

    /* Pattern change is too large. */
    if (pattern_diff < -3 || pattern_diff > 3) {
        return;
    }

    /* Determine if we need to increment or decrement. We will track when the encoder decrements but we don't show
     * decrements at the output. This is done because the split-flap mechanism can't physically undo a flap, and the
     * encoder position is used to keep track of which flap is displayed. */

    if (ctx->encoder.digital[ENC_CH_Z] & !zero_lockout) {
        encoder_zero(ctx);
        backspin_prevention = 0;
        zero_lockout        = true;
    } else if (pattern_diff < 0) {
        backspin_prevention += pattern_diff;
    } else {
        while (pattern_diff-- != 0) {
            if (backspin_prevention < 0) {
                backspin_prevention++;
            } else {
                encoder_increment(ctx);
                /* Make sure we are well past 1 so we don't flipper between 1 and 0. */
                if (ctx->flap_position > 5) {
                    zero_lockout = false;
                }
            }
        }
    }

    /* Update the old_pattern. */
    old_pattern = new_pattern;
}

//----------------------------------------------------------------------------------------------------------------------

void distance_update(of_ctx_t *ctx)
{
    int16_t distance = flapIndex_wrap_calc((int16_t)(ctx->flap_setpoint) - ctx->flap_position);
    /* Check if a short rotation needs to be extended. */
    if (ctx->extend_revolution) {
        if (distance < ctx->of_config.minimum_rotation) {
            distance += ENCODER_PULSES_PER_REVOLUTION;
        } else {
            ctx->extend_revolution = false;
        }
    }
    ctx->flap_distance = distance;
}

//----------------------------------------------------------------------------------------------------------------------

void motor_state_update(of_ctx_t *ctx)
{
    if (ctx->of_hal.motor.speed > 0) {
        ctx->motor_active_timeout_tick = of_hal_tick_count_get() + MOTOR_IDLE_TIMEOUT_MS;
        if (!ctx->motor_active) {
            ctx->motor_active = true;
            debug_io_log_info("Motor Active\n");
        }
    } else if (ctx->motor_active && of_hal_tick_count_get() > ctx->motor_active_timeout_tick) {
        ctx->motor_active = false;
        debug_io_log_info("Motor Idle\n");
    }
}

//----------------------------------------------------------------------------------------------------------------------

void comms_state_update(of_ctx_t *ctx)
{
    if (chain_comm_is_busy(&ctx->chain_ctx)) {
        ctx->comms_active_timeout_tick = of_hal_tick_count_get() + COMMS_IDLE_TIMEOUT_MS;
        if (!ctx->comms_active) {
            ctx->comms_active = true;
            debug_io_log_info("Comms Active\n");
#if !CHAIN_COMM_DEBUG
            debug_io_log_set_level(LOG_LVL_WARN); /* Writing to RTT may fuck up the UART RX interrupt... */
#endif
        }
    } else if (ctx->comms_active && of_hal_tick_count_get() > ctx->comms_active_timeout_tick) {
        ctx->comms_active = false;
#if !CHAIN_COMM_DEBUG
        debug_io_log_restore();
#endif
        debug_io_log_info("Comms Idle\n");
    }
}

//----------------------------------------------------------------------------------------------------------------------

void encoder_increment(of_ctx_t *ctx)
{
    ctx->flap_position_prev = ctx->flap_position;
    ctx->flap_position      = flapIndex_wrap_calc(ctx->flap_position + 1);
    distance_update(ctx);
}

//----------------------------------------------------------------------------------------------------------------------

void encoder_zero(of_ctx_t *ctx)
{
    ctx->flap_position_prev = ctx->flap_position;
    ctx->flap_position      = ctx->of_config.encoder_offset;
    distance_update(ctx);
}

//----------------------------------------------------------------------------------------------------------------------

void motor_control_loop(of_ctx_t *ctx, uint32_t cl_tick)
{
    int32_t pid_pwm          = 0; /* Motor pwm value as calculated by the PID. */
    int32_t feed_forward_pwm = 0; /* Motor pwm value from feed-forward control. */
    int32_t cl_pwm           = 0; /* Motor pwm value as calculated by the control loop. */

    /* Compute the PID and feed forward. */
    if (ctx->encoder_rps_x100_setpoint == 0) {
        ctx->pid_ctx.integral = 0;
    }

    pid_pwm          = pid_compute(&ctx->pid_ctx, ctx->encoder_rps_x100_setpoint - ctx->encoder_rps_x100_actual, 1000);
    feed_forward_pwm = interp_compute(&ctx->speed_pwm_interp_ctx, ctx->encoder_rps_x100_setpoint);

    cl_pwm = pid_pwm + feed_forward_pwm; /* PWM value calculated by the control loop. */

    /* Set motor direction and speed. */
    if (ctx->debug_flags.motor_control_override == false) {
        if (cl_pwm < 0) {
            ctx->of_hal.motor.mode  = MOTOR_REVERSE;
            ctx->of_hal.motor.speed = -cl_pwm;
        } else if (cl_pwm > 0) {
            ctx->of_hal.motor.mode  = MOTOR_FORWARD;
            ctx->of_hal.motor.speed = cl_pwm;
        } else {
            ctx->of_hal.motor.mode  = MOTOR_BRAKE; // Set to brake if speed is zero
            ctx->of_hal.motor.speed = 0;
        }
        // ctx->motor_backspin_timeout_tick = 0;
        // } else {
        //     ctx->pid_ctx.integral = 0;
        //     // if (ctx->motor_backspin_timeout_tick == 0) {
        //     //     ctx->motor_backspin_timeout_tick = cl_tick + MOTOR_BACKSPIN_DURATION_TICKS;
        //     //     ctx->of_hal.motor.mode           = MOTOR_REVERSE;
        //     //     ctx->of_hal.motor.speed          = MOTOR_BACKSPIN_PWM;
        //     // } else if (cl_tick >= ctx->motor_backspin_timeout_tick) {
        //     ctx->of_hal.motor.mode  = MOTOR_BRAKE; // Set to brake if speed is zero
        //     ctx->of_hal.motor.speed = 0;
        //     // }
        // }

        /* Skip if we are in manual control mode. */
        of_hal_motor_control(&ctx->of_hal.motor);
    }

    if (cl_tick % 8 == 1) {
        debug_io_scope_t debug_io_scope = {
            .setpoint     = ctx->encoder_rps_x100_setpoint,
            .actual       = ctx->encoder_rps_x100_actual,
            .p            = ctx->pid_ctx.p_error,
            .i            = ctx->pid_ctx.i_error,
            .d            = ctx->pid_ctx.d_error,
            .pid_out      = pid_pwm,
            .feed_forward = feed_forward_pwm,
            .cl_out       = cl_pwm,
        };
        debug_io_scope_push(&debug_io_scope, sizeof(debug_io_scope));
    }
}

void of_encoder_speed_calc(of_ctx_t *ctx, uint32_t sens_tick)
{
    /* Calculate the average actual speed. */
    if (ctx->flap_position != ctx->flap_position_prev) {
        int16_t flap_position_delta    = flapIndex_wrap_calc((int16_t)ctx->flap_position - ctx->flap_position_prev);
        int32_t flap_change_time_delta = (int32_t)sens_tick - ctx->flap_position_change_tick_prev;
        int32_t ticks_per_rev          = ENCODER_PULSES_PER_REVOLUTION * flap_change_time_delta / flap_position_delta;
        ctx->encoder_rps_x100_actual =
            (ctx->encoder_rps_x100_actual * 19 + (100000 / ticks_per_rev)) / 20; /* Average 20 last samples. */
        // if (ctx->encoder_rps_x100_actual > 40) {
        //     debug_io_log_error("position %d - %d = %d\n", ctx->flap_position, ctx->flap_position_prev,
        //                        flap_position_delta);
        //     debug_io_log_error("ticks %d - %d = %d\n", sens_tick, ctx->flap_position_change_tick_prev,
        //                        flap_change_time_delta);
        //     debug_io_log_error("tpr %d / rps %d\n", ticks_per_rev, ctx->encoder_rps_x100_actual);
        // }
        ctx->flap_position_change_tick_prev = sens_tick;
        ctx->flap_position_prev             = ctx->flap_position;
    } else if (sens_tick >= ctx->flap_position_change_tick_prev + 135) {
        /* If no position change for a while, reset the average speed */
        ctx->encoder_rps_x100_actual = 0;
        // ctx->flap_position_change_tick_prev = sens_tick;
    }
}

void of_speed_setpoint_set_from_distance(of_ctx_t *ctx)
{
    /* Don't update setpoint if we are in manual override. */
    if (ctx->debug_flags.rps_x100_setpoint_override) {
        return;
    }

    /* Determine the speed setpoint */
    ctx->encoder_rps_x100_setpoint = interp_compute(&ctx->distance_speed_interp_ctx, ctx->flap_distance);
    if (ctx->flap_distance < ENCODER_PULSES_PER_SYMBOL) {
        ctx->encoder_rps_x100_setpoint = 0;
    }
}

void of_encoder_sensor_calibration_start(of_ctx_t *ctx)
{
    /* Start the calibration process. */
    ctx->encoder_sensor_calibration_ticks = ENCODER_SENSOR_CALIBRATION_DURATION_TICKS;
    for (uint8_t i = 0; i < ENCODER_CHANNEL_COUNT; i++) {
        ctx->encoder.min[i] = UINT16_MAX;
        ctx->encoder.max[i] = 0;
    }

    ctx->of_hal.motor.speed = ENCODER_SENSOR_CALIBRATION_MOTOR_PWM;
    ctx->of_hal.motor.mode  = MOTOR_FORWARD;
    of_hal_motor_control(&ctx->of_hal.motor);
}

void of_encoder_sensor_calibration_loop(of_ctx_t *ctx)
{
    for (uint8_t i = 0; i < ENCODER_CHANNEL_COUNT; i++) {
        if (ctx->encoder.analog[i] < ctx->encoder.min[i]) {
            ctx->encoder.min[i] = ctx->encoder.analog[i];
        }
        if (ctx->encoder.analog[i] > ctx->encoder.max[i]) {
            ctx->encoder.max[i] = ctx->encoder.analog[i];
        }
    }

    if (--ctx->encoder_sensor_calibration_ticks == 0) {
        ctx->of_hal.motor.speed = 0;
        ctx->of_hal.motor.mode  = MOTOR_BRAKE;
        of_hal_motor_control(&ctx->of_hal.motor);
        debug_io_log_info("Encoder calibration complete. Min/Max values:\n");
        for (uint8_t i = 0; i < ENCODER_CHANNEL_COUNT; i++) {
            ctx->of_config.enc_cal[i].min = ctx->encoder.min[i] /*+ (ctx->encoder.max[i] - ctx->encoder.min[i]) / 20*/;
            ctx->of_config.enc_cal[i].max = ctx->encoder.max[i] /*- (ctx->encoder.max[i] - ctx->encoder.min[i]) / 20*/;
            debug_io_log_info("Channel %u: min=%u, max=%u\n", i, ctx->of_config.enc_cal[i].min,
                              ctx->of_config.enc_cal[i].max);
        }
        ctx->store_config = true; // Store the new calibration values.
    }
}

bool of_encoder_sensor_calibration_ongoing(of_ctx_t *ctx)
{
    return ctx->encoder_sensor_calibration_ticks > 0;
}