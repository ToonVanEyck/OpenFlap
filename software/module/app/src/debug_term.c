#include "debug_term.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//======================================================================================================================
//                                                  FUNCTION PROTOTYPES
//======================================================================================================================

static void debug_term_config_dump(int argc, char *argv[], void *userdata);
static void debug_term_test_uart(int argc, char *argv[], void *userdata);
static void debug_term_test_motor(int argc, char *argv[], void *userdata);
static void debug_term_test_adc(int argc, char *argv[], void *userdata);
static void debug_term_position_setpoint_set(int argc, char *argv[], void *userdata);
static void debug_term_speed_setpoint_set(int argc, char *argv[], void *userdata);
static void debug_term_pid_tune(int argc, char *argv[], void *userdata);
static void debug_term_ir_lims_set(int argc, char *argv[], void *userdata);
static void debug_term_i_lim_update(int argc, char *argv[], void *userdata);
static void debug_term_control_loop_toggle(int argc, char *argv[], void *userdata);
static void debug_term_led(int argc, char *argv[], void *userdata);
static void debug_term_info(int argc, char *argv[], void *userdata);

//======================================================================================================================
//                                                   PUBLIC FUNCTIONS
//======================================================================================================================

void debug_term_init(of_ctx_t *of_ctx)
{
    simple_term_init();

    simple_term_register_keyword("config", debug_term_config_dump, of_ctx);
    simple_term_register_keyword("uart", debug_term_test_uart, of_ctx);
    simple_term_register_keyword("motor", debug_term_test_motor, of_ctx);
    simple_term_register_keyword("adc", debug_term_test_adc, of_ctx);
    simple_term_register_keyword("p_sp", debug_term_position_setpoint_set, of_ctx);
    simple_term_register_keyword("s_sp", debug_term_speed_setpoint_set, of_ctx);
    simple_term_register_keyword("pid", debug_term_pid_tune, of_ctx);
    simple_term_register_keyword("ir", debug_term_ir_lims_set, of_ctx);
    simple_term_register_keyword("i_lim", debug_term_i_lim_update, of_ctx);
    simple_term_register_keyword("cl", debug_term_control_loop_toggle, of_ctx);
    simple_term_register_keyword("led", debug_term_led, of_ctx);
    simple_term_register_keyword("info", debug_term_info, of_ctx);
}

//======================================================================================================================
//                                                         PRIVATE FUNCTIONS
//======================================================================================================================

/**
 * @brief Dumps the current configuration to the debug terminal.
 *
 * @param[in] argc     Number of arguments.
 * @param[in] argv     Argument values.
 * @param[in] userdata A pointer to the openflap configuration context.
 */
static void debug_term_config_dump(int argc, char *argv[], void *userdata)
{
    (void)argc;
    (void)argv;

    of_config_t *config = (of_config_t *)userdata;
    printf("Config:\n");
    printf("Encoder offset: %d\n", config->encoder_offset);
    printf("IR thresholds: %d %d\n", config->ir_threshold.lower, config->ir_threshold.upper);
    printf("Base speed: %d\n", config->base_speed);
    printf("Symbol set: ");
    for (int i = 0; i < SYMBOL_CNT; i++) {
        printf("%s ", (char *)&config->symbol_set[i]);
    }
    printf("\n");
    printf("Minimum rotation: %d\n", config->minimum_rotation);
    printf("Foreground Color: %ld\n", config->color.foreground);
    printf("Background Color: %ld\n", config->color.background);
    printf("Motion config: %d %d %d %d\n", config->motion.min_pwm, config->motion.max_pwm,
           config->motion.min_ramp_distance, config->motion.max_ramp_distance);
    printf("\n");
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * @brief Sends a test message to the UART driver.
 *
 * @param[in] argc     Number of arguments.
 * @param[in] argv     Argument values.
 * @param[in] userdata A pointer to the openflap configuration context.
 */
static void debug_term_test_uart(int argc, char *argv[], void *userdata)
{
    of_ctx_t *of_ctx = (of_ctx_t *)userdata;

    if (argc != 2) {
        printf("Usage: %s <message>\n", argv[0]);
        return;
    }

    printf("Sending %d bytes over uart: %s\n", (int)strlen(argv[1]), argv[1]);

    uart_driver_write(&of_ctx->of_hal.uart_driver, (uint8_t *)argv[1], strlen(argv[1]));
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * @brief Tests the motor control by setting speed and decay mode.
 *
 * @param[in] argc     Number of arguments.
 * @param[in] argv     Argument values.
 * @param[in] userdata A pointer to the openflap configuration context.
 */
static void debug_term_test_motor(int argc, char *argv[], void *userdata)
{
    of_ctx_t *of_ctx = (of_ctx_t *)userdata;

    if (argc == 3) {
        int speed = atoi(argv[1]);
        int decay = atoi(argv[2]);

        if (speed >= 0 && speed <= 1000 && decay >= 0 && decay <= 1000) {
            printf("Setting motor speed to %d and decay to %d\n", speed, decay);
            of_ctx->motor_control_override = true; // Override motor control to use fixed speeds.
            of_hal_motor_control(speed, decay);
            return;
        }
    }

    printf("Usage: %s <speed (0-1000)> <decay (0-1000)>\n", argv[0]);
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * @brief Toggles the printing of the raw ADC encoder values.
 *
 * @param[in] argc     Number of arguments.
 * @param[in] argv     Argument values.
 * @param[in] userdata A pointer to the openflap configuration context.
 */
static void debug_term_test_adc(int argc, char *argv[], void *userdata)
{
    (void)argc;
    (void)argv;

    of_ctx_t *of_ctx = (of_ctx_t *)userdata;

    of_ctx->debug_flags.print_adc_values = !of_ctx->debug_flags.print_adc_values; // Toggle ADC values printing
    printf("ADC values printing %s\n", of_ctx->debug_flags.print_adc_values ? "enabled" : "disabled");
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * @brief Sets the flap setpoint based on user input.
 *
 * @param[in] argc     Number of arguments.
 * @param[in] argv     Argument values.
 * @param[in] userdata A pointer to the openflap configuration context.
 */
static void debug_term_position_setpoint_set(int argc, char *argv[], void *userdata)
{
    of_ctx_t *of_ctx = (of_ctx_t *)userdata;

    if (argc == 2) {
        int setpoint = atoi(argv[1]);
        if (setpoint >= 0 && setpoint < SYMBOL_CNT) {
            of_ctx->flap_setpoint = setpoint * ENCODER_PULSES_PER_SYMBOL;
            distance_update(of_ctx);
            if (of_ctx->flap_distance < of_ctx->of_config.minimum_rotation) {
                of_ctx->extend_revolution = true;
                of_ctx->flap_distance += ENCODER_PULSES_PER_REVOLUTION;
            }
            printf("Setpoint updated to %d\n", setpoint);
            return;
        }
    }

    printf("Usage: %s <setpoint (0-%d)>\n", argv[0], SYMBOL_CNT - 1);
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * @brief Sets the speed setpoint based on user input.
 *
 * @param[in] argc     Number of arguments.
 * @param[in] argv     Argument values.
 * @param[in] userdata A pointer to the openflap configuration context.
 */
static void debug_term_speed_setpoint_set(int argc, char *argv[], void *userdata)
{
    of_ctx_t *of_ctx = (of_ctx_t *)userdata;

    if (argc == 2) {
        int setpoint = atoi(argv[1]);
        if (setpoint >= 0 && setpoint <= 120) {
            of_ctx->encoder_rps_x100_setpoint              = setpoint;
            of_ctx->debug_flags.rps_x100_setpoint_override = true;
            of_ctx->motor_control_override                 = false;
            printf("Setpoint updated to %d\n", setpoint);
            return;
        }
    }

    printf("Usage: %s <setpoint (0-120)>\n", argv[0]);
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * @brief Tunes the PID controller parameters based on user input.
 *
 * @param[in] argc     Number of arguments.
 * @param[in] argv     Argument values.
 * @param[in] userdata A pointer to the openflap configuration context.
 */

static void debug_term_pid_tune(int argc, char *argv[], void *userdata)
{
    of_ctx_t *of_ctx = (of_ctx_t *)userdata;

    if (argc == 4) {
        int32_t kp = atoi(argv[1]);
        int32_t ki = atoi(argv[2]);
        int32_t kd = atoi(argv[3]);

        of_ctx->pid_ctx.kp = kp;
        of_ctx->pid_ctx.ki = ki;
        of_ctx->pid_ctx.kd = kd;

        printf("PID updated: kp=%ld, ki=%ld, kd=%ld\n", kp, ki, kd);
        return;
    }

    printf("Usage: %s <kp> <ki> <kd>\n", argv[0]);
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * @brief Sets the IR sensor thresholds based on user input.
 *
 * @param[in] argc     Number of arguments.
 * @param[in] argv     Argument values.
 * @param[in] userdata A pointer to the openflap configuration context.
 */

static void debug_term_ir_lims_set(int argc, char *argv[], void *userdata)
{
    of_ctx_t *of_ctx = (of_ctx_t *)userdata;

    if (argc == 3) {
        int16_t base                         = atoi(argv[1]);
        int16_t range                        = atoi(argv[2]);
        of_ctx->of_config.ir_threshold.lower = base - range;
        of_ctx->of_config.ir_threshold.upper = base + range;
        printf("IR thresholds updated: lower=%d, upper=%d\n", base - range, base + range);
        return;
    }

    printf("Usage: %s <base> <range>\n", argv[0]);
}

//----------------------------------------------------------------------------------------------------------------------

static void debug_term_i_lim_update(int argc, char *argv[], void *userdata)
{
    of_ctx_t *of_ctx = (of_ctx_t *)userdata;

    if (argc == 2) {
        int32_t i_lim = atoi(argv[1]);
        pid_i_lim_update(&of_ctx->pid_ctx, -i_lim, i_lim);
        printf("PID integral limits updated: i_lim=%ld\n", i_lim);
        return;
    }

    printf("Usage: %s <i_lim>\n", argv[0]);
}

//----------------------------------------------------------------------------------------------------------------------

static void debug_term_control_loop_toggle(int argc, char *argv[], void *userdata)
{
    (void)argc;
    (void)argv;

    of_ctx_t *of_ctx = (of_ctx_t *)userdata;

    of_ctx->motor_control_override                 = false;
    of_ctx->debug_flags.rps_x100_setpoint_override = false;

    printf("Motor control loop enabled\n");
}

//----------------------------------------------------------------------------------------------------------------------

static void debug_term_led(int argc, char *argv[], void *userdata)
{

    if (argc == 1) {
        of_hal_led_toggle();
        return;
    }
    if (argc == 2) {
        int value = atoi(argv[1]);
        if (value == 0 || value == 1) {
            of_hal_led_set(value);
            return;
        }
    }
    printf("Usage: %s [value (0|1)]\n", argv[0]);
}

//------------------------------------------------------------------------------------------------------------------------

/**
 * @brief Print some interesting debug information.
 *
 * @param[in] argc     Number of arguments.
 * @param[in] argv     Argument values.
 * @param[in] userdata A pointer to the openflap context.
 */
static void debug_term_info(int argc, char *argv[], void *userdata)
{
    (void)argc;
    (void)argv;

    of_ctx_t *ctx = (of_ctx_t *)userdata;

    printf("Col-End      : %s\n", of_hal_is_column_end() ? "Yes" : "No");
    printf("12V OK       : %s\n", of_hal_is_12V_ok() ? "Ok" : "Not Ok");
    printf("Control Loop : %s\n", ctx->motor_control_override ? "Manual" : "Auto");
}