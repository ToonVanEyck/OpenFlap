/* Includes ------------------------------------------------------------------*/
// #include "chain_comm.h"
#include "config.h"
#include "interpolation.h"
#include "openflap.h"
#include "peripherals.h"
#include "property_handlers.h"

#include <stdio.h>
#include <stdlib.h>

/* Private define ------------------------------------------------------------*/

/** Convert a milliseconds value into a counter value for the IR/Encoder timer. */
#define IR_TIMER_TICKS_FROM_MS(ms) ((ms) * 10)
/** Convert a microsecond value into a counter value for the IR/Encoder timer. */
#define IR_TIMER_TICKS_FROM_US(us) ((us) / 100)

/** The period of the encoder readings when the motor is idle. */
#define IR_IDLE_PERIOD_MS IR_TIMER_TICKS_FROM_MS(50)
/** The period of the encoder readings when the motor is active. */
#define IR_ACTIVE_PERIOD_MS IR_TIMER_TICKS_FROM_MS(1)

/** The IR sensor will illuminate the encoder wheel for this time in microseconds before starting the conversion */
#define IR_ILLUMINATE_TIME_US IR_TIMER_TICKS_FROM_US(200)

#ifndef VERSION
#define VERSION "not found"
#endif

/* Type definitions ---------------------------------------------------------*/

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

/* Private variables ---------------------------------------------------------*/

static openflap_ctx_t openflap_ctx = {0};
peripherals_ctx_t peripherals_ctx  = {0}; /**< The context for peripherals. */

static bool adc_values_print = false; /**< Flag to indicate if the ADC values should be printed. */

extern uint32_t checksum; /**< The checksum of the firmware, used for versioning. */

static debug_io_scope_t debug_io_scope = {0}; /**< The debug IO scope for PID debugging. */

static uint32_t speed_sp = 0;

/** Interpolation table to get PWM value based on desired motor speed. Used for feed forward control. */
static const interp_point_t speed_pwm_table[] = {
    {0, 0}, {1, 100}, {20, 120}, {34, 150}, {50, 200}, {75, 400}, {88, 600}, {100, 1000}, /* {100x_RPS , PWM}, ... */
};
static interp_ctx_t speed_pwm_interp_ctx = {0}; /**< Interpolation context for motor speed control */

static const interp_point_t distance_speed_table[] = {
    {4, 20}, {10, 60},
    /* {Distance , 100x_RPS}, ... */
};
static interp_ctx_t distance_speed_interp_ctx = {0}; /**< Interpolation context for motor distance control */

/* Private macro -------------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

/* Debug terminal functions. */
void debug_term_config_dump(const char *input, void *arg);
void debug_term_test_uart(const char *input, void *arg);
void debug_term_test_motor(const char *input, void *arg);
void debug_term_test_adc(const char *input, void *arg);
void debug_term_setpoint_set(const char *input, void *arg);
void debug_term_pid_tune(const char *input, void *arg);
void debug_term_ir_lims_set(const char *input, void *arg);
void debug_term_i_lim_update(const char *input, void *arg);
void debug_term_test_interp(const char *input, void *arg);

int main(void)
{
    peripherals_init(&peripherals_ctx);

    configLoad(&openflap_ctx.config);

    /* Initialize debugging features.*/
    debug_io_init(LOG_LVL_DEBUG);
    debug_io_scope_init("u4u4i4i4i4i4i4i4");

    debug_io_term_register_keyword("config", debug_term_config_dump, &openflap_ctx.config);
    debug_io_term_register_keyword("uart", debug_term_test_uart, NULL);
    debug_io_term_register_keyword("motor", debug_term_test_motor, NULL);
    debug_io_term_register_keyword("adc", debug_term_test_adc, NULL);
    debug_io_term_register_keyword("sp", debug_term_setpoint_set, NULL);
    debug_io_term_register_keyword("pid", debug_term_pid_tune, NULL);
    debug_io_term_register_keyword("ir", debug_term_ir_lims_set, NULL);
    debug_io_term_register_keyword("i_lim", debug_term_i_lim_update, NULL);
    debug_io_term_register_keyword("interp", debug_term_test_interp, NULL);

    /* Initialize chain communication and property handlers. */
    chain_comm_init(&openflap_ctx.chain_ctx, &peripherals_ctx.uart_driver);
    property_handlers_init(&openflap_ctx);

    /* Initialize the PID controller. */
    pid_init(&openflap_ctx.pid_ctx, 1200, 6, 0);
    pid_o_lim_update(&openflap_ctx.pid_ctx, -1000, 1000);
    pid_i_lim_update(&openflap_ctx.pid_ctx, -75, 75);

    /* Initialize interpolation context for motor control */
    interp_init(&speed_pwm_interp_ctx, speed_pwm_table, sizeof(speed_pwm_table) / sizeof(speed_pwm_table[0]));

    /* Initialize interpolation context for motor distance control */
    interp_init(&distance_speed_interp_ctx, distance_speed_table,
                sizeof(distance_speed_table) / sizeof(distance_speed_table[0]));

    /* Print boot messages. */
    uint32_t checksum_be = ((checksum & 0x000000FF) << 24) | ((checksum & 0x0000FF00) << 8) |
                           ((checksum & 0x00FF0000) >> 8) | ((checksum & 0xFF000000) >> 24);
    debug_io_log_info("App Name : OpenFlap module\n");
    debug_io_log_info("Version  : %s \n", GIT_VERSION);
    debug_io_log_info("CRC      : %08x\n", checksum_be);

    /* Start homing sequence at maximum speed. */
    openflap_ctx.flap_position = 0;          /* Invalid position. */
    openflap_ctx.flap_setpoint = 0;          /* Setpoint zero means, go to the first character in the character map. */
    openflap_ctx.flap_distance = SYMBOL_CNT; /* Speed is based on distance between setpoint and position. We initialize
                                                it with the maximum value. */

    uint32_t pwm_ticker_prev = 0, pwm_ticker_curr = 0, pwm_ticker_prev_pos_change = 0;
    encoder_states_t encoder_states = {false, false, false};
    uint8_t flap_position_prev = 0, flap_position_curr = 0;
    uint32_t ticks_per_rev = 0; /* Ticks per revolution. */
    uint32_t rps_x100_avg  = 0; /* Rotations per 100 seconds. */

    while (1) {
        /* Handle the RTT terminal. */
        debug_io_term_process();

        /* If the module is the last module in a column, a secondary uart TX will route this modules data back to the
         * module above it. Normally this data would come from the module below it. */
        uart_tx_pin_update(is_column_end());

        /* Run chain comm. */
        chain_comm(&openflap_ctx.chain_ctx);

        /* Update the PWM timer tick count. */
        pwm_ticker_curr = get_pwm_tick_count();
        if (pwm_ticker_curr != pwm_ticker_prev) {
            pwm_ticker_prev = pwm_ticker_curr;

            /* Calculate the new position. */
            encoder_states_get(&encoder_states, openflap_ctx.config.ir_threshold.lower,
                               openflap_ctx.config.ir_threshold.upper);
            encoder_position_update(&openflap_ctx, &encoder_states);

            // from_distance_motor_set(&openflap_ctx, &peripherals_ctx.motor);

            if (pwm_ticker_curr % 4 == 0 && adc_values_print) {
                debug_io_log_info("raw: %s%04ld\x1b[0m %s%04ld\x1b[0m %s%04ld\x1b[0m\n",
                                  encoder_states.enc_a ? "\x1b[7m" : "\x1b[27m", encoder_states.enc_raw_a,
                                  encoder_states.enc_b ? "\x1b[7m" : "\x1b[27m", encoder_states.enc_raw_b,
                                  encoder_states.enc_z ? "\x1b[7m" : "\x1b[27m", encoder_states.enc_raw_z);
            }

            speed_sp = interp_compute(&distance_speed_interp_ctx, openflap_ctx.flap_distance);
            if (openflap_ctx.flap_distance == 0) {
                speed_sp = 0; // If the distance is zero, set speed to zero
            }

            flap_position_curr = flap_position_get(&openflap_ctx);
            if (flap_position_curr != flap_position_prev) {
                int distance = flap_position_curr - flap_position_prev;
                distance     = distance < 0 ? distance + SYMBOL_CNT : distance;

                flap_position_prev         = flap_position_curr;
                ticks_per_rev              = SYMBOL_CNT * (pwm_ticker_curr - pwm_ticker_prev_pos_change) / distance;
                rps_x100_avg               = (rps_x100_avg * 7 + (20000 / ticks_per_rev)) / 8;
                pwm_ticker_prev_pos_change = pwm_ticker_curr;
                // debug_io_log_info("fp: %d (%s)\n", flap_position_curr,
                // &openflap_ctx.config.symbol_set[flap_position_curr]);
            } else if (pwm_ticker_curr >= pwm_ticker_prev_pos_change + 30) {
                // If no position change for a while, reset the average speed
                rps_x100_avg = 0;
            }

            int32_t pid_output = pid_compute(&openflap_ctx.pid_ctx, speed_sp - rps_x100_avg, 1000);
            int32_t ff_speed   = interp_compute(&speed_pwm_interp_ctx, speed_sp);

            if (speed_sp == 0) {
                openflap_ctx.pid_ctx.integral = 0;
                pid_output                    = 0;
                ff_speed                      = 0;
            }

            int32_t cl_output = pid_output + ff_speed;

            if (cl_output < 0) {
                peripherals_ctx.motor.mode  = MOTOR_REVERSE;
                peripherals_ctx.motor.speed = -cl_output;
            } else if (cl_output > 0) {
                peripherals_ctx.motor.mode  = MOTOR_FORWARD;
                peripherals_ctx.motor.speed = cl_output;
            } else {
                peripherals_ctx.motor.mode  = MOTOR_BRAKE; // Set to brake if speed is zero
                peripherals_ctx.motor.speed = 0;
            }
            motor_control(&peripherals_ctx.motor);

            if (pwm_ticker_curr % 8 == 1) {
                debug_io_scope.setpoint     = speed_sp;
                debug_io_scope.actual       = rps_x100_avg;
                debug_io_scope.p            = openflap_ctx.pid_ctx.p_error;
                debug_io_scope.i            = openflap_ctx.pid_ctx.i_error;
                debug_io_scope.d            = openflap_ctx.pid_ctx.d_error;
                debug_io_scope.pid_out      = pid_output;
                debug_io_scope.feed_forward = ff_speed;
                debug_io_scope.cl_out       = cl_output;
                debug_io_scope_push(&debug_io_scope, sizeof(debug_io_scope));
            }
        }

        /* Update the flap position. */
        // flap_position_curr = flap_position_get(&openflap_ctx);
        // if (flap_position_curr != flap_position_prev) {
        //     flap_position_prev = flap_position_curr;
        //     debug_io_log_info("fp: %d (%s)\n", flap_position_curr,
        //     &openflap_ctx.config.symbol_set[flap_position_curr]);
        // }

        /* Set debug pins based on flap position. */
        debug_pin_set(0, flap_position_get(&openflap_ctx) & 1);
        debug_pin_set(1, flap_position_get(&openflap_ctx) == 0);

        /* Communication status. */
        comms_state_update(&openflap_ctx);

        /* Motor status. */
        motor_state_update(&openflap_ctx);

        /* Idle logic. */
        if (!openflap_ctx.motor_active && !openflap_ctx.comms_active) {
            if (openflap_ctx.store_config) {
                openflap_ctx.store_config = false;
                configStore(&openflap_ctx.config);
                debug_io_log_info(0, "Config stored!\n");
            }
            if (openflap_ctx.reboot) {
                debug_io_log_info(0, "Rebooting module!\n");
                NVIC_SystemReset();
            }
        }
    }
}

void APP_ErrorHandler(void)
{
    while (1)
        ;
}

void debug_term_config_dump(const char *input, void *arg)
{
    openflap_config_t *config = (openflap_config_t *)arg;
    configPrint(config);
}

void debug_term_test_uart(const char *input, void *arg)
{
    (void)arg; // Unused argument
    uart_driver_write(&peripherals_ctx.uart_driver, (uint8_t *)input, strlen(input));
}

void debug_term_test_motor(const char *input, void *arg)
{
    (void)arg; // Unused argument

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
        peripherals_ctx.motor.mode  = MOTOR_REVERSE;
        peripherals_ctx.motor.speed = -speed;
    } else if (speed > 0) {
        peripherals_ctx.motor.mode  = MOTOR_FORWARD;
        peripherals_ctx.motor.speed = speed;
    } else {
        peripherals_ctx.motor.mode  = MOTOR_IDLE; // Set to idle if speed is zero
        peripherals_ctx.motor.speed = 0;
    }

    if (decay == 's' || decay == 'S') {
        peripherals_ctx.motor.decay_mode = MOTOR_DECAY_SLOW;
    } else if (decay == 'f' || decay == 'F') {
        peripherals_ctx.motor.decay_mode = MOTOR_DECAY_FAST;
    } else if (decay == 'b' || decay == 'B') {
        peripherals_ctx.motor.mode = MOTOR_BRAKE; // Special case for brake mode
    } else if (decay == 'i' || decay == 'I') {
        peripherals_ctx.motor.mode = MOTOR_IDLE; // Special case for idle mode
    }

    motor_control(&peripherals_ctx.motor);
}

void debug_term_test_adc(const char *input, void *arg)
{
    (void)arg;   // Unused argument
    (void)input; // Unused argument

    adc_values_print = !adc_values_print; // Toggle ADC values printing
}

void debug_term_setpoint_set(const char *input, void *arg)
{
    (void)arg; // Unused argument

    if (input && strlen(input) > 0) {
        int setpoint = atoi(input);
        if (setpoint >= 0 && setpoint < SYMBOL_CNT) {
            openflap_ctx.flap_setpoint = setpoint;
            distance_update(&openflap_ctx);
            if (openflap_ctx.flap_distance < openflap_ctx.config.minimum_rotation) {
                openflap_ctx.extend_revolution = true;
                openflap_ctx.flap_distance += SYMBOL_CNT;
            }
            debug_io_log_info("Setpoint updated to %d\n", setpoint);
        } else {
            debug_io_log_error("Invalid setpoint value. Must be between 0 and %d.\n", SYMBOL_CNT - 1);
        }
    } else {
        debug_io_log_error("No input provided for setpoint.\n");
    }

    // (void)arg; // Unused argument

    // if (input && strlen(input) > 0) {
    //     int setpoint = atoi(input);
    //     if (setpoint >= 0 && setpoint <= 100) {
    //         speed_sp = setpoint;
    //         debug_io_log_info("Setpoint updated to %d\n", setpoint);
    //     } else {
    //         debug_io_log_error("Invalid setpoint value. Must be between 0 and %d.\n", 100);
    //     }
    // } else {
    //     debug_io_log_error("No input provided for setpoint.\n");
    // }
}

void debug_term_pid_tune(const char *input, void *arg)
{
    (void)arg; // Unused argument

    int32_t kp = 0, ki = 0, kd = 0;
    if (input && sscanf(input, "%ld %ld %ld", &kp, &ki, &kd) == 3) {
        openflap_ctx.pid_ctx.kp = kp;
        openflap_ctx.pid_ctx.ki = ki;
        openflap_ctx.pid_ctx.kd = kd;
        debug_io_log_info("PID updated: kp=%d, ki=%d, kd=%d\n", kp, ki, kd);
    } else {
        debug_io_log_error("Invalid input. Usage: pid <kp> <ki> <kd>\n");
    }
}

void debug_term_ir_lims_set(const char *input, void *arg)
{
    (void)arg; // Unused argument

    if (input && strlen(input) > 0) {
        uint16_t base = 0, range = 0;
        if (sscanf(input, "%hu %hu", &base, &range) == 2) {
            openflap_ctx.config.ir_threshold.lower = base - range;
            openflap_ctx.config.ir_threshold.upper = base + range;
            debug_io_log_info("IR thresholds updated: lower=%d, upper=%d\n", base - range, base + range);
        } else {
            debug_io_log_error("Invalid input format. Expected: <lower> <upper>\n");
        }
    } else {
        debug_io_log_error("No input provided for IR thresholds.\n");
    }
}

void debug_term_i_lim_update(const char *input, void *arg)
{
    (void)arg; // Unused argument

    if (input && strlen(input) > 0) {
        int32_t i_lim = 0;
        if (sscanf(input, "%ld", &i_lim) == 1) {
            pid_i_lim_update(&openflap_ctx.pid_ctx, -i_lim, i_lim);
            debug_io_log_info("PID integral limits updated: i_lim=%d\n", i_lim);
        } else {
            debug_io_log_error("Invalid input format. Expected: <i_lim>\n");
        }
    } else {
        debug_io_log_error("No input provided for PID integral limits.\n");
    }
}

void debug_term_test_interp(const char *input, void *arg)
{
    (void)arg; // Unused argument

    if (input && strlen(input) > 0) {
        int32_t x_value = 0;
        if (sscanf(input, "%ld", &x_value) == 1) {
            int32_t result = interp_compute(&speed_pwm_interp_ctx, x_value);
            debug_io_log_info("Interpolation: x=%ld -> y=%ld\n", x_value, result);
        } else {
            debug_io_log_error("Invalid input format. Expected: <x_value>\n");
        }
    } else {
        debug_io_log_error("No input provided for interpolation test.\n");
    }
}