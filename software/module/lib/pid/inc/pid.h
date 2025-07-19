#pragma once

#include <stddef.h>
#include <stdint.h>

typedef struct {
    int32_t kp;
    int32_t ki;
    int32_t kd;
    int32_t setpoint;
    int32_t integral;
    int32_t previous_error;
    int32_t i_min; // Minimum value for integral term (anti-windup)
    int32_t i_max; // Maximum value for integral term (anti-windup)
    int32_t p_error;
    int32_t i_error;
    int32_t d_error;
    int32_t output; // PID output value
} pid_ctx_t;

void pid_init(pid_ctx_t *pid, int32_t kp, int32_t ki, int32_t kd);

int32_t pid_compute(pid_ctx_t *pid, int32_t error);

void pid_i_lim_update(pid_ctx_t *pid, int32_t i_min, int32_t i_max);