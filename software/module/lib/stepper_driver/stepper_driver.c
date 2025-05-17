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
static uint32_t calculate_real_step_from_virtual_step(stepper_driver_ctx_t *ctx, uint32_t virtual_step);
static uint32_t calculate_virtual_step_from_real_step(stepper_driver_ctx_t *ctx, uint32_t real_step);

void stepper_driver_init(stepper_driver_ctx_t *ctx, uint32_t tick_period_us, uint32_t steps_per_revolution,
                         uint32_t virtual_steps_per_revolution, stepper_driver_set_pins_cb_t set_pins_cb,
                         stepper_driver_dynamic_speed_cb_t speed_cb)
{
    ctx->tick_period_us       = tick_period_us;
    ctx->steps_per_revolution = steps_per_revolution;
    ctx->set_pins_cb          = set_pins_cb;
    ctx->speed_cb             = speed_cb;
    ctx->virtual_steps_per_revolution =
        virtual_steps_per_revolution ? virtual_steps_per_revolution : steps_per_revolution;
    ctx->direction           = STEPPER_DRIVER_DIR_CW;
    ctx->mode_sequence_idx   = 0;
    ctx->use_dynamic_speed   = false;
    ctx->rps_x100            = 0;
    ctx->ticks_per_step      = 0;
    ctx->ticks_cnt           = 0;
    ctx->step_cnt            = 0;
    ctx->step_cnt_target     = 0;
    ctx->full_rotations_todo = 0;
    stepper_driver_mode_set(ctx, STEPPER_DRIVER_MODE_WAVE_DRIVE);
}

bool stepper_driver_tick(stepper_driver_ctx_t *ctx)
{
    bool step_complete = false;

    /* Check if the driver is initialized. */
    if (ctx->set_pins_cb == NULL) {
        return false;
    }

    /* Stop the motor. */
    if ((ctx->step_cnt == ctx->step_cnt_target && ctx->full_rotations_todo == 0) || ctx->ticks_per_step == 0) {
        ctx->set_pins_cb(ctx, false, false, false, false);
        return false;
    }

    /* Set the pins according to the current mode sequence. */
    ctx->set_pins_cb(ctx, ctx->mode_sequence[ctx->mode_sequence_idx][0], ctx->mode_sequence[ctx->mode_sequence_idx][1],
                     ctx->mode_sequence[ctx->mode_sequence_idx][2], ctx->mode_sequence[ctx->mode_sequence_idx][3]);

    /* Decrement the step period counter. */
    if (ctx->ticks_cnt > 0) {
        ctx->ticks_cnt--;
    } else {
        /* Reset the tick counter. */
        ctx->ticks_cnt = ctx->ticks_per_step;

        /* Increment or decrement the sequence index according to drive direction. */
        ctx->mode_sequence_idx += (ctx->direction == STEPPER_DRIVER_DIR_CW) ? 1 : -1;
        ctx->mode_sequence_idx &= ctx->mode_sequence_size - 1;

        /* Increment the step count. */
        if (ctx->step_cnt_target != STEPPER_DRIVER_STEP_INFINITE) {
            ctx->step_cnt += (ctx->direction == STEPPER_DRIVER_DIR_CW) ? 1 : -1;
            ctx->step_cnt =
                (((ctx->step_cnt % ctx->steps_per_revolution) + ctx->steps_per_revolution) % ctx->steps_per_revolution);
            if (ctx->step_cnt == ctx->step_cnt_target && ctx->full_rotations_todo > 0) {
                ctx->full_rotations_todo--;
            }
        }

        /* Recalculate the speed parameters. */
        stepper_driver_calc_speed_params(ctx);

        /* Set the step complete flag. */
        step_complete = true;
    }
    return step_complete;
}

void stepper_driver_speed_set(stepper_driver_ctx_t *ctx, uint16_t rps_x100)
{
    if (rps_x100 == STEPPER_DRIVER_SPEED_DYNAMIC) {
        ctx->use_dynamic_speed = true;
    } else {
        ctx->use_dynamic_speed = false;
        ctx->rps_x100          = rps_x100;
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

void stepper_driver_rotate(stepper_driver_ctx_t *ctx, uint32_t step_cnt)
{
    if (step_cnt == STEPPER_DRIVER_STEP_INFINITE) {
        ctx->step_cnt_target = STEPPER_DRIVER_STEP_INFINITE;
    } else {
        ctx->full_rotations_todo = step_cnt / ctx->virtual_steps_per_revolution;
        step_cnt                 = step_cnt % ctx->virtual_steps_per_revolution;
        stepper_driver_position_set(ctx, stepper_driver_position_get(ctx) + step_cnt);
    }
}

void stepper_driver_virtual_step_offset_set(stepper_driver_ctx_t *ctx, uint32_t offset)
{
    uint32_t step            = calculate_virtual_step_from_real_step(ctx, ctx->step_cnt_target);
    ctx->virtual_step_offset = offset;
    stepper_driver_position_set(ctx, step);
}

uint32_t stepper_driver_position_get(stepper_driver_ctx_t *ctx)
{
    return calculate_virtual_step_from_real_step(ctx, ctx->step_cnt_target);
}

void stepper_driver_position_set(stepper_driver_ctx_t *ctx, uint32_t position)
{
    ctx->step_cnt_target = calculate_real_step_from_virtual_step(ctx, position);
}

void stepper_driver_home_set(stepper_driver_ctx_t *ctx)
{
    ctx->step_cnt = 0;
}

bool stepper_driver_is_spinning(stepper_driver_ctx_t *ctx)
{
    return (ctx->step_cnt != ctx->step_cnt_target || ctx->full_rotations_todo > 0);
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
        ctx->rps_x100 = ctx->speed_cb(ctx, ctx->step_cnt, ctx->step_cnt_target);
    }

    /* If the speed is 0, then stop the motor. */
    if (ctx->rps_x100 == 0) {
        ctx->ticks_per_step = 0;
        return;
    }

    /* 1000000 because of conversion of s -> us. */
    /* 100 because of rps_x100. */
    ctx->ticks_per_step = (1000000 * 100) / (ctx->tick_period_us * ctx->steps_per_revolution * ctx->rps_x100);
}

static uint32_t calculate_real_step_from_virtual_step(stepper_driver_ctx_t *ctx, uint32_t virtual_step)
{
    int32_t steps = 0;

    /* Convert to real steps. */
    steps = (virtual_step * ctx->steps_per_revolution) / ctx->virtual_steps_per_revolution;
    /* Apply offset. */
    steps = steps - ctx->virtual_step_offset;
    /* Wrap around the number of steps. */
    steps = (steps + ctx->steps_per_revolution) % ctx->steps_per_revolution;

    return steps;
}

static uint32_t calculate_virtual_step_from_real_step(stepper_driver_ctx_t *ctx, uint32_t real_step)
{
    int32_t steps = 0;

    /* Apply offset */
    steps = real_step + ctx->virtual_step_offset;
    /* Wrap around number of steps. */
    steps = ((steps + ctx->steps_per_revolution) % ctx->steps_per_revolution);
    /* Convert to virtual steps. Round by adding 0.5 in the form of steps_per_revolution and multiplying the rest
     * by 2... */
    steps =
        (2 * steps * ctx->virtual_steps_per_revolution + ctx->steps_per_revolution) / (2 * ctx->steps_per_revolution);

    return steps;
}