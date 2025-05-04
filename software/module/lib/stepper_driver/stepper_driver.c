#include "stepper_driver.h"

const uint8_t stepper_wave_drive_size        = 4;
const bool stepper_wave_drive_sequence[4][4] = {
    {true, false, false, false},
    {false, true, false, false},
    {false, false, true, false},
    {false, false, false, true},
};

const uint8_t stepper_full_drive_size        = 4;
const bool stepper_full_drive_sequence[4][4] = {
    {true, false, false, true},
    {true, true, false, false},
    {false, true, true, false},
    {false, false, true, true},
};

const uint8_t stepper_half_drive_size        = 8;
const bool stepper_half_drive_sequence[8][4] = {
    {true, false, false, true}, {true, false, false, false}, {true, true, false, false}, {false, true, false, false},
    {false, true, true, false}, {false, false, true, false}, {false, false, true, true}, {false, false, false, true},
};

static void stepper_driver_calc_speed_params(stepper_driver_ctx_t *ctx);

void stepper_driver_init(stepper_driver_ctx_t *ctx, uint32_t tick_period_us, uint32_t steps_per_revolution,
                         stepper_driver_set_pins_cb_t set_pins_cb, stepper_driver_dynamic_speed_cb_t speed_cb)
{
    ctx->tick_period_us       = tick_period_us;
    ctx->steps_per_revolution = steps_per_revolution;
    ctx->set_pins_cb          = set_pins_cb;
    ctx->speed_cb             = speed_cb;
    ctx->direction            = STEPPER_DRIVER_DIR_CW;
    ctx->mode_sequence_idx    = 0;
    ctx->use_dynamic_speed    = false;
    ctx->rps_x10              = 0;
    ctx->ticks_per_step       = 0;
    ctx->ticks_cnt            = 0;
    ctx->step_cnt             = 0;
    ctx->step_cnt_target      = 0;
    stepper_driver_mode_set(ctx, STEPPER_DRIVER_MODE_WAVE_DRIVE);
}

void stepper_driver_tick(stepper_driver_ctx_t *ctx)
{
    /* Check if the driver is initialized. */
    if (ctx->set_pins_cb == NULL) {
        return;
    }

    /* Stop the motor. */
    if (ctx->step_cnt_target == 0 || ctx->ticks_per_step == 0) {
        ctx->set_pins_cb(ctx, false, false, false, false);
        return;
    }

    /* Decrement the step period counter. */
    if (ctx->ticks_cnt > 0) {
        ctx->ticks_cnt--;
    } else {
        /* Reset the tick counter. */
        ctx->ticks_cnt = ctx->ticks_per_step;

        /* Increment or decrement the sequence index according to drive direction. */
        ctx->mode_sequence_idx += (ctx->direction == STEPPER_DRIVER_DIR_CW) ? 1 : -1;
        ctx->mode_sequence_idx &= ctx->mode_sequence_size - 1;

        /* Set the pins according to the current mode sequence. */
        ctx->set_pins_cb(ctx, ctx->mode_sequence[ctx->mode_sequence_idx][0],
                         ctx->mode_sequence[ctx->mode_sequence_idx][1], ctx->mode_sequence[ctx->mode_sequence_idx][2],
                         ctx->mode_sequence[ctx->mode_sequence_idx][3]);

        /* Increment the step count. */
        if (ctx->step_cnt_target != STEPPER_DRIVER_DEGREES_INFINITY) {
            ctx->step_cnt++;
        }

        /* Check if we reached the target step count. */
        if (ctx->step_cnt >= ctx->step_cnt_target) {
            ctx->step_cnt        = 0;
            ctx->step_cnt_target = 0;
        }

        /* Recalculate the speed parameters. */
        stepper_driver_calc_speed_params(ctx);
    }
}

void stepper_driver_speed_set(stepper_driver_ctx_t *ctx, uint16_t rps_x10)
{
    if (rps_x10 == STEPPER_DRIVER_SPEED_DYNAMIC) {
        ctx->use_dynamic_speed = true;
    } else {
        ctx->use_dynamic_speed = false;
        ctx->rps_x10           = rps_x10;
    }
    stepper_driver_calc_speed_params(ctx);
}

void stepper_driver_dir_set(stepper_driver_ctx_t *ctx, stepper_driver_dir_t dir)
{
    if (dir == STEPPER_DRIVER_DIR_CW || dir == STEPPER_DRIVER_DIR_CCW) {
        ctx->direction = dir;
    }
}

void stepper_driver_mode_set(stepper_driver_ctx_t *ctx, stepper_driver_mode_t mode)
{
    if (mode >= STEPPER_DRIVER_MODE_WAVE_DRIVE && mode <= STEPPER_DRIVER_MODE_HALF_DRIVE) {
        ctx->mode = mode;
        switch (mode) {
            case STEPPER_DRIVER_MODE_WAVE_DRIVE:
                ctx->mode_sequence      = stepper_wave_drive_sequence;
                ctx->mode_sequence_size = stepper_wave_drive_size;
                break;
            case STEPPER_DRIVER_MODE_FULL_DRIVE:
                ctx->mode_sequence      = stepper_full_drive_sequence;
                ctx->mode_sequence_size = stepper_full_drive_size;
                break;
            case STEPPER_DRIVER_MODE_HALF_DRIVE:
                ctx->mode_sequence      = stepper_half_drive_sequence;
                ctx->mode_sequence_size = stepper_half_drive_size;
                break;
        }
    }
}

void stepper_driver_degrees_rotate(stepper_driver_ctx_t *ctx, uint32_t degrees_x10)
{
    if (degrees_x10 == STEPPER_DRIVER_DEGREES_INFINITY) {
        ctx->step_cnt_target = STEPPER_DRIVER_DEGREES_INFINITY;
    } else {
        ctx->step_cnt_target = (degrees_x10 * ctx->steps_per_revolution) / 3600;
    }
}

/**
 * \brief Calculate the speed parameters.
 *
 * This function calculates the number of ticks per step based on the requested rotations per second. If the driver is
 * configured to use a callback to change the speed every step, then this callback is called to get the speed.
 */
static void stepper_driver_calc_speed_params(stepper_driver_ctx_t *ctx)
{
    if (ctx->use_dynamic_speed && ctx->speed_cb != NULL) {
        ctx->rps_x10 = ctx->speed_cb(ctx, ctx->step_cnt, ctx->step_cnt_target);
    }

    /* If the speed is 0, then stop the motor. */
    if (ctx->rps_x10 == 0) {
        ctx->ticks_per_step = 0;
        return;
    }

    /* 1000000 because of conversion of s -> us. */
    /* 10 because of rps_x10. */
    ctx->ticks_per_step = (1000000 * 10) / (ctx->tick_period_us * ctx->steps_per_revolution * ctx->rps_x10);
}