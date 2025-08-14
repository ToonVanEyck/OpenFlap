#include "debug_term.h"
#include "debug_io.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//======================================================================================================================
//                                                  FUNCTION PROTOTYPES
//======================================================================================================================

static void debug_term_config_dump(const char *input, void *arg);
static void debug_term_test_uart(const char *input, void *arg);
static void debug_term_test_motor(const char *input, void *arg);
static void debug_term_test_adc(const char *input, void *arg);
static void debug_term_position_setpoint_set(const char *input, void *arg);
static void debug_term_speed_setpoint_set(const char *input, void *arg);
static void debug_term_pid_tune(const char *input, void *arg);
static void debug_term_ir_lims_set(const char *input, void *arg);
static void debug_term_i_lim_update(const char *input, void *arg);
static void debug_term_control_loop_toggle(const char *input, void *arg);
static void debug_term_encoder_calibration(const char *input, void *arg);

//======================================================================================================================
//                                                   PUBLIC FUNCTIONS
//======================================================================================================================

void debug_term_init(of_ctx_t *of_ctx)
{
    debug_io_init(LOG_LVL_DEBUG);
    debug_io_scope_init("u4u4i4i4i4i4i4i4");

    debug_io_term_register_keyword("config", debug_term_config_dump, of_ctx);
    debug_io_term_register_keyword("uart", debug_term_test_uart, of_ctx);
    debug_io_term_register_keyword("motor", debug_term_test_motor, of_ctx);
    debug_io_term_register_keyword("adc", debug_term_test_adc, of_ctx);
    debug_io_term_register_keyword("p_sp", debug_term_position_setpoint_set, of_ctx);
    debug_io_term_register_keyword("s_sp", debug_term_speed_setpoint_set, of_ctx);
    debug_io_term_register_keyword("pid", debug_term_pid_tune, of_ctx);
    debug_io_term_register_keyword("ir", debug_term_ir_lims_set, of_ctx);
    debug_io_term_register_keyword("i_lim", debug_term_i_lim_update, of_ctx);
    debug_io_term_register_keyword("cl", debug_term_control_loop_toggle, of_ctx);
    debug_io_term_register_keyword("enc_cal", debug_term_encoder_calibration, of_ctx);
}

//======================================================================================================================
//                                                         PRIVATE FUNCTIONS
//======================================================================================================================

/**
 * @brief Dumps the current configuration to the debug terminal.
 *
 * @param[in] input The input string from the debug terminal (unused).
 * @param[in] arg   A pointer to the openflap configuration context.
 */
static void debug_term_config_dump(const char *input, void *arg)
{
    of_config_t *config = (of_config_t *)arg;
    debug_io_log_info("Config:\n");
    debug_io_log_info("Encoder offset: %d\n", config->encoder_offset);
    debug_io_log_info("Encoder calibration:\n");
    for (int i = 0; i < ENCODER_CHANNEL_COUNT; i++) {
        debug_io_log_info("  Channel %d: %d %d\n", i, config->enc_cal[i].min, config->enc_cal[i].max);
    }
    debug_io_log_info("IR thresholds: %d %d\n", config->ir_threshold.lower, config->ir_threshold.upper);
    debug_io_log_info("Base speed: %d\n", config->base_speed);
    debug_io_log_info("Symbol set:\n");
    for (int i = 0; i < SYMBOL_CNT; i++) {
        debug_io_log_info("%s ", &config->symbol_set[i]);
    }
    debug_io_log_info("Minimum rotation: %d\n", config->minimum_rotation);
    debug_io_log_info("Foreground Color: %d\n", config->color.foreground);
    debug_io_log_info("Background Color: %d\n", config->color.background);
    debug_io_log_info("Motion config: %d %d %d %d\n", config->motion.min_pwm, config->motion.max_pwm,
                      config->motion.min_ramp_distance, config->motion.max_ramp_distance);
    debug_io_log_info("\n");
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * @brief Sends a test message to the UART driver.
 *
 * @param[in] input String to write to UART.
 * @param[in] arg   A pointer to the uart driver.
 */
static void debug_term_test_uart(const char *input, void *arg)
{
    of_ctx_t *of_ctx = (of_ctx_t *)arg;

    uart_driver_write(&of_ctx->of_hal.uart_driver, (uint8_t *)input, strlen(input));
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * @brief Tests the motor control by setting speed and decay mode.
 *
 * @param[in] input String containing speed and decay mode. e.g. "500 s" for 500 speed with slow decay.
 * @param[in] arg   A pointer to the motor context.
 */
static void debug_term_test_motor(const char *input, void *arg)
{
    of_ctx_t *of_ctx = (of_ctx_t *)arg;

    // Parse speed and decay mode
    int speed  = 0;
    char decay = 0; // Default to fast decay

    // Expect input format: "<speed> <decay s/f>"
    if (input) {
        // Try to parse speed and decay
        int parsed = sscanf(input, "%d %c", &speed, &decay);
        if (parsed < 1) {
            debug_io_log_error("Invalid input format. Expected: <speed> <decay s/f>\n");
            return;
        }
    }

    if (speed < 0) {
        of_ctx->of_hal.motor.mode  = MOTOR_REVERSE;
        of_ctx->of_hal.motor.speed = -speed;
    } else if (speed > 0) {
        of_ctx->of_hal.motor.mode  = MOTOR_FORWARD;
        of_ctx->of_hal.motor.speed = speed;
    } else {
        of_ctx->of_hal.motor.mode  = MOTOR_IDLE; // Set to idle if speed is zero
        of_ctx->of_hal.motor.speed = 0;
    }

    if (decay == 's' || decay == 'S') {
        debug_io_log_debug("Motor mode set to SLOW_DECAY\n");
        of_ctx->of_hal.motor.decay_mode = MOTOR_DECAY_SLOW;
    } else if (decay == 'f' || decay == 'F') {
        debug_io_log_debug("Motor mode set to FAST_DECAY\n");
        of_ctx->of_hal.motor.decay_mode = MOTOR_DECAY_FAST;
    } else if (decay == 'b' || decay == 'B') {
        debug_io_log_debug("Motor mode set to BRAKE\n");
        of_ctx->of_hal.motor.mode = MOTOR_BRAKE; // Special case for brake mode
    } else if (decay == 'i' || decay == 'I') {
        debug_io_log_debug("Motor mode set to IDLE\n");
        of_ctx->of_hal.motor.mode = MOTOR_IDLE; // Special case for idle mode
    }

    of_ctx->debug_flags.motor_control_override = true; /* Indicate that the motor pwm is fixed. */

    of_hal_motor_control(&of_ctx->of_hal.motor);
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * @brief Toggles the printing of the raw ADC encoder values.
 *
 * @param[in] input The input string from the debug terminal (unused).
 * @param[in] arg   A pointer to a boolean flag to toggle ADC printing.
 */
static void debug_term_test_adc(const char *input, void *arg)
{
    (void)input; // Unused argument

    of_ctx_t *of_ctx = (of_ctx_t *)arg;

    of_ctx->debug_flags.print_adc_values = !of_ctx->debug_flags.print_adc_values; // Toggle ADC values printing
    debug_io_log_debug("ADC values printing %s\n", of_ctx->debug_flags.print_adc_values ? "enabled" : "disabled");
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * @brief Sets the flap setpoint based on user input.
 *
 * @param[in] input The input string from the debug terminal, expected to be a number.
 * @param[in] arg   A pointer to the openflap context.
 */
static void debug_term_position_setpoint_set(const char *input, void *arg)
{
    of_ctx_t *of_ctx = (of_ctx_t *)arg;

    if (input && strlen(input) > 0) {
        int setpoint = atoi(input);
        if (setpoint >= 0 && setpoint < SYMBOL_CNT) {
            of_ctx->flap_setpoint = setpoint * ENCODER_PULSES_PER_SYMBOL;
            distance_update(of_ctx);
            if (of_ctx->flap_distance < of_ctx->of_config.minimum_rotation) {
                of_ctx->extend_revolution = true;
                of_ctx->flap_distance += ENCODER_PULSES_PER_REVOLUTION;
            }
            debug_io_log_info("Setpoint updated to %d\n", setpoint);
        } else {
            debug_io_log_error("Invalid setpoint value. Must be between 0 and %d.\n", SYMBOL_CNT - 1);
        }
    } else {
        debug_io_log_error("No input provided for setpoint.\n");
    }
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * @brief Sets the speed setpoint based on user input.
 *
 * @param[in] input The input string from the debug terminal, expected to be a number.
 * @param[in] arg   A pointer to the openflap context.
 */
static void debug_term_speed_setpoint_set(const char *input, void *arg)
{
    of_ctx_t *of_ctx = (of_ctx_t *)arg;

    if (input && strlen(input) > 0) {
        int setpoint = atoi(input);
        if (setpoint >= 0 && setpoint <= 120) {
            of_ctx->encoder_rps_x100_setpoint              = setpoint;
            of_ctx->debug_flags.rps_x100_setpoint_override = true;
            of_ctx->debug_flags.motor_control_override     = false;
            debug_io_log_info("Setpoint updated to %d\n", setpoint);
        } else {
            debug_io_log_error("Invalid setpoint value. Must be between 0 and %d.\n", 120);
        }
    } else {
        debug_io_log_error("No input provided for setpoint.\n");
    }
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * @brief Tunes the PID controller parameters based on user input.
 *
 * @param[in] input The input string from the debug terminal, expected to be in the format "kp ki kd".
 * @param[in] arg   A pointer to the PID context.
 */

static void debug_term_pid_tune(const char *input, void *arg)
{
    of_ctx_t *of_ctx = (of_ctx_t *)arg;

    int32_t kp = 0, ki = 0, kd = 0;
    if (input && sscanf(input, "%ld %ld %ld", &kp, &ki, &kd) == 3) {
        of_ctx->pid_ctx.kp = kp;
        of_ctx->pid_ctx.ki = ki;
        of_ctx->pid_ctx.kd = kd;
        debug_io_log_info("PID updated: kp=%d, ki=%d, kd=%d\n", kp, ki, kd);
    } else {
        debug_io_log_error("Invalid input. Usage: pid <kp> <ki> <kd>\n");
    }
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * @brief Sets the IR sensor thresholds based on user input.
 *
 * @param[in] input The input string from the debug terminal, expected to be in the format "base range".
 * @param[in] arg   A pointer to the openflap configuration context.
 */

static void debug_term_ir_lims_set(const char *input, void *arg)
{
    of_ctx_t *of_ctx = (of_ctx_t *)arg;

    if (input && strlen(input) > 0) {
        uint16_t base = 0, range = 0;
        if (sscanf(input, "%hu %hu", &base, &range) == 2) {
            of_ctx->of_config.ir_threshold.lower = base - range;
            of_ctx->of_config.ir_threshold.upper = base + range;
            debug_io_log_info("IR thresholds updated: lower=%d, upper=%d\n", base - range, base + range);
        } else {
            debug_io_log_error("Invalid input format. Expected: <lower> <upper>\n");
        }
    } else {
        debug_io_log_error("No input provided for IR thresholds.\n");
    }
}

//----------------------------------------------------------------------------------------------------------------------

static void debug_term_i_lim_update(const char *input, void *arg)
{
    of_ctx_t *of_ctx = (of_ctx_t *)arg;

    if (input && strlen(input) > 0) {
        int32_t i_lim = 0;
        if (sscanf(input, "%ld", &i_lim) == 1) {
            pid_i_lim_update(&of_ctx->pid_ctx, -i_lim, i_lim);
            debug_io_log_info("PID integral limits updated: i_lim=%d\n", i_lim);
        } else {
            debug_io_log_error("Invalid input format. Expected: <i_lim>\n");
        }
    } else {
        debug_io_log_error("No input provided for PID integral limits.\n");
    }
}

//----------------------------------------------------------------------------------------------------------------------

static void debug_term_control_loop_toggle(const char *input, void *arg)
{
    (void)input; // Unused argument

    of_ctx_t *of_ctx = (of_ctx_t *)arg;

    of_ctx->debug_flags.motor_control_override     = false;
    of_ctx->debug_flags.rps_x100_setpoint_override = false;

    debug_io_log_debug("Motor control loop enabled\n");
}

//----------------------------------------------------------------------------------------------------------------------

static void debug_term_encoder_calibration(const char *input, void *arg)
{
    of_ctx_t *of_ctx = (of_ctx_t *)arg;

    of_encoder_sensor_calibration_start(of_ctx);
}