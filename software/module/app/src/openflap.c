#include "openflap.h"
#include "debug_io.h"

#define MOTOR_IDLE_TIMEOUT_MS (500)
#define COMMS_IDLE_TIMEOUT_MS (75)

/** The motor will reverse direction for this duration after reaching the desired flap. */
#define MOTOR_BACKSPIN_DURATION_TICKS (25)
/** The motor will revers direction with this pwm value after reaching the desired flap. */
#define MOTOR_BACKSPIN_PWM (350)

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
    int32_t decay;
    // uint32_t pwm;
    // uint32_t avg_speed;
} debug_io_scope_t;

//----------------------------------------------------------------------------------------------------------------------

void of_encoder_values_update(of_ctx_t *ctx)
{
    /* Get the new values from the adc. */
    of_hal_encoder_values_get(ctx->encoder.analog);

    /* Digitize the analog values. */
    for (uint8_t i = 0; i < ENCODER_CHANNEL_COUNT; i++) {
        if (ctx->encoder.analog[i] < ctx->of_config.ir_threshold.lower) {
            ctx->encoder.digital[i] = true;
        } else if (ctx->encoder.analog[i] > ctx->of_config.ir_threshold.upper) {
            ctx->encoder.digital[i] = false;
        }
    }
}

//----------------------------------------------------------------------------------------------------------------------

void of_encoder_position_update(of_ctx_t *ctx)
{
    static const uint8_t QEM[16] = {0, -1, 1, 2, 1, 0, 2, -1, -1, 2, 0, 1, 2, 1, -1, 0};

    /* Prevent decrementing the flap position, instead wind up the backspin prevention counter. */
    static int8_t backspin_prevention = -SYMBOL_CNT;
    /* Prevent jumping back from 1 to zero. */
    static bool zero_lockout = false;
    /* Encoder increment/decrement is determined based on the old and new pattern. */
    static uint8_t old_pattern = 0x00;

    /* Digitize the ADC signals using the configured IR thresholds. */
    uint8_t new_pattern = (ctx->encoder.digital[ENC_CH_A] << 1) | (ctx->encoder.digital[ENC_CH_B] << 0);

    int8_t qem = QEM[(old_pattern << 2) | new_pattern];

    /* Update the old_pattern. */
    old_pattern = new_pattern;

    if (qem == 2) {
        debug_io_log_error("Invalid QEM value: %d\n", qem);
        return;
    }

    /* Check if we have a zero or full pattern. */
    if (ctx->encoder.digital[ENC_CH_Z] & !zero_lockout) {
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
    if (of_hal_motor_is_running()) {
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
    int32_t pid_output         = 0; /* Motor speed value as calculated by the PID. */
    int32_t feed_forward_speed = 0; /* Motor speed value from feed-forward control. */
    int32_t cl_speed           = 0; /* Motor speed value as calculated by the control loop. */

    /* Compute the PID and feed forward. */
    if (ctx->encoder_rps_x100_setpoint == 0) {
        ctx->pid_ctx.integral = 0;
    }

    int32_t decay_setpoint =
        (ctx->encoder_rps_x100_setpoint == 0)
            ? 1000
            : interpolation_linear_compute(&ctx->sd_interpolation_ctx, ctx->encoder_rps_x100_setpoint);

    pid_output = pid_compute(&ctx->pid_ctx, ctx->encoder_rps_x100_setpoint - ctx->encoder_rps_x100_actual, 1000);
    // feed_forward_speed = interp_compute(&ctx->speed_pwm_interp_ctx, ctx->encoder_rps_x100_setpoint);
    feed_forward_speed =
        interpolation_bilinear_compute(&ctx->sdp_interpolation_ctx, ctx->encoder_rps_x100_setpoint, decay_setpoint);

    cl_speed = feed_forward_speed;

    if (pid_output < 0) {
        decay_setpoint -= 4 * pid_output; /* If pid output is negative, we need to increase the decay. */
    } else {
        cl_speed += pid_output; /* If pid output is positive, we need to increase the speed. */
    }

    /* Set motor direction and speed. */
    /* Skip if we are in manual control mode. */
    if (ctx->debug_flags.motor_control_override == false) {
        if (ctx->flap_distance == 0 && ctx->flap_distance_prev != 0) {
            ctx->motor_backspin_timeout_tick = MOTOR_BACKSPIN_DURATION_TICKS;
        } else if (ctx->motor_backspin_timeout_tick) {
            ctx->motor_backspin_timeout_tick--;
            cl_speed       = -MOTOR_BACKSPIN_PWM;
            decay_setpoint = 1000;
        } else if (ctx->flap_distance == 0) {
            cl_speed       = 0;
            decay_setpoint = 1000;
        }
        ctx->flap_distance_prev = ctx->flap_distance;
        of_hal_motor_control(cl_speed, decay_setpoint);
    }

    if (cl_tick % 10 == 1) {
        debug_io_scope_t debug_io_scope = {
            .setpoint     = ctx->encoder_rps_x100_setpoint,
            .actual       = ctx->encoder_rps_x100_actual,
            .p            = ctx->pid_ctx.p_error,
            .i            = ctx->pid_ctx.i_error,
            .d            = ctx->pid_ctx.d_error,
            .pid_out      = pid_output,
            .feed_forward = feed_forward_speed,
            .cl_out       = cl_speed,
            .decay        = decay_setpoint,
        };
        debug_io_scope_push(&debug_io_scope, sizeof(debug_io_scope));
    }
}

//----------------------------------------------------------------------------------------------------------------------

void of_encoder_speed_calc(of_ctx_t *ctx, uint32_t sens_tick)
{
    /* Calculate the average actual speed. */
    if (ctx->flap_position != ctx->flap_position_prev) {
        int16_t flap_position_delta    = flapIndex_wrap_calc((int16_t)ctx->flap_position - ctx->flap_position_prev);
        int32_t flap_change_time_delta = (int32_t)sens_tick - ctx->flap_position_change_tick_prev;
        int32_t ticks_per_rev          = ENCODER_PULSES_PER_REVOLUTION * flap_change_time_delta / flap_position_delta;
        ctx->encoder_rps_x100_actual   = (ctx->encoder_rps_x100_actual * 7 + (100000 / ticks_per_rev)) / 8;
        // if (ctx->encoder_rps_x100_actual > 1000) {
        //     debug_io_log_error("position %d - %d = %d\n", ctx->flap_position, ctx->flap_position_prev,
        //                        flap_position_delta);
        //     debug_io_log_error("ticks %d - %d = %d\n", sens_tick, ctx->flap_position_change_tick_prev,
        //                        flap_change_time_delta);
        //     debug_io_log_error("tpr %d / rps %d\n", ticks_per_rev, ctx->encoder_rps_x100_actual);
        // }
        ctx->flap_position_change_tick_prev = sens_tick;
        ctx->flap_position_prev             = ctx->flap_position;
    } else if (sens_tick >= ctx->flap_position_change_tick_prev + 200) {
        /* If no position change for a while, reset the average speed */
        ctx->encoder_rps_x100_actual = 0;
    }
}

//----------------------------------------------------------------------------------------------------------------------

void of_speed_setpoint_set_from_distance(of_ctx_t *ctx)
{
    /* Don't update setpoint if we are in manual override. */
    if (ctx->debug_flags.rps_x100_setpoint_override) {
        return;
    }

    /* Determine the speed setpoint */
    ctx->encoder_rps_x100_setpoint = interpolation_linear_compute(&ctx->ds_interpolation_ctx, ctx->flap_distance);
    if (ctx->flap_distance < ENCODER_PULSES_PER_SYMBOL) {
        ctx->encoder_rps_x100_setpoint = 0;
    }
}
