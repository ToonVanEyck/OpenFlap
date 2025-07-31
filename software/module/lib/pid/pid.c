#include "pid.h"

#define GAIN_SCALE 1000 // Scale factor for gains to avoid floating point operations

void pid_init(pid_ctx_t *pid, int32_t kp, int32_t ki, int32_t kd)
{
    pid->kp             = kp;
    pid->ki             = ki;
    pid->kd             = kd;
    pid->setpoint       = 0;
    pid->integral       = 0;
    pid->previous_error = 0;
    pid->i_min          = -100000;
    pid->i_max          = 100000;
    pid->o_min          = -100000; // Set default min output
    pid->o_max          = 100000;  // Set default max output
}

int32_t pid_compute(pid_ctx_t *pid, int32_t error, int32_t dt_us)
{
    // Calculate new integral with error
    pid->integral += error;

    int32_t derivative  = error - pid->previous_error;
    pid->previous_error = error;

    pid->p_error = (pid->kp * error);
    pid->i_error = (pid->ki * pid->integral);
    pid->d_error = (pid->kd * derivative);

    // Anti-windup: Check scaled integral value against limits
    if (pid->i_error > pid->i_max) {
        // Back-calculate the raw integral value that would produce max
        pid->integral = pid->i_max / pid->ki;
    } else if (pid->i_error < pid->i_min) {
        // Back-calculate the raw integral value that would produce min
        pid->integral = pid->i_min / pid->ki;
    }
    pid->i_error = (pid->ki * pid->integral);

    pid->output = (pid->p_error + pid->i_error + pid->d_error) / GAIN_SCALE;

    // Clamp output to min/max limits
    if (pid->output < pid->o_min) {
        pid->output = pid->o_min;
    } else if (pid->output > pid->o_max) {
        pid->output = pid->o_max;
    }

    return pid->output;
}

void pid_i_lim_update(pid_ctx_t *pid, int32_t i_min, int32_t i_max)
{
    pid->i_min = i_min * GAIN_SCALE;
    pid->i_max = i_max * GAIN_SCALE;

    pid->i_error = (pid->ki * pid->integral);
    // Anti-windup: Check scaled integral value against limits
    if (pid->i_error > pid->i_max) {
        // Back-calculate the raw integral value that would produce max
        pid->integral = pid->i_max / pid->ki;
    } else if (pid->i_error < pid->i_min) {
        // Back-calculate the raw integral value that would produce min
        pid->integral = pid->i_min / pid->ki;
    }
}

void pid_o_lim_update(pid_ctx_t *pid, int32_t o_min, int32_t o_max)
{
    pid->o_min = o_min;
    pid->o_max = o_max;

    // Ensure the output is within the new limits
    if (pid->output < pid->o_min) {
        pid->output = pid->o_min;
    } else if (pid->output > pid->o_max) {
        pid->output = pid->o_max;
    }
}
