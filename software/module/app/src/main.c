/* Includes ------------------------------------------------------------------*/
// #include "chain_comm.h"
#include "config.h"
#include "openflap.h"
#include "peripherals.h"
#include "pid.h"
#include "property_handlers.h"
#include "uart_driver.h"

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

/* Private variables ---------------------------------------------------------*/

static openflap_ctx_t openflap_ctx = {0};
peripherals_ctx_t peripherals_ctx  = {0}; /**< The context for peripherals. */

static bool adc_values_print = false; /** Flag to indicate if the ADC values should be printed. */

extern uint32_t checksum; /** The checksum of the firmware, used for versioning. */
/* Private macro -------------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

/* Debug terminal functions. */
void debug_term_config_dump(const char *input, void *arg);
void debug_term_test_uart(const char *input, void *arg);
void debug_term_test_motor(const char *input, void *arg);
void debug_term_test_adc(const char *input, void *arg);

int main(void)
{
    peripherals_init(&peripherals_ctx);

    configLoad(&openflap_ctx.config);

    /* Initialize debugging features.*/
    debug_io_init(LOG_LVL_DEBUG);

    debug_io_term_register_keyword("config", debug_term_config_dump, &openflap_ctx.config);
    debug_io_term_register_keyword("uart", debug_term_test_uart, NULL);
    debug_io_term_register_keyword("motor", debug_term_test_motor, NULL);
    debug_io_term_register_keyword("adc", debug_term_test_adc, NULL);

    /* Initialize chain communication and property handlers. */
    chain_comm_init(&openflap_ctx.chain_ctx, &peripherals_ctx.uart_driver);
    property_handlers_init(&openflap_ctx);

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

    uint32_t pwm_ticker_prev = 0, pwm_ticker_curr = 0;
    encoder_states_t encoder_states = {false, false, false};
    uint8_t flap_position_prev = 0, flap_position_curr = 0;
    while (1) {
        /* Handle the RTT terminal. */
        debug_io_term_process();

        /* Run chain comm. */
        chain_comm(&openflap_ctx.chain_ctx);

        /* Update the PWM timer tick count. */
        pwm_ticker_curr = get_pwm_tick_count();
        if (pwm_ticker_curr != pwm_ticker_prev) {
            pwm_ticker_prev = pwm_ticker_curr;

            encoder_states_get(&encoder_states, openflap_ctx.config.ir_threshold.lower,
                               openflap_ctx.config.ir_threshold.upper);
            encoder_position_update(&openflap_ctx, &encoder_states);

            if (pwm_ticker_curr % 25 == 0 && adc_values_print) {
                debug_io_log_info("raw: %s%04ld\x1b[0m %s%04ld\x1b[0m %s%04ld\x1b[0m\n",
                                  encoder_states.enc_a ? "\x1b[7m" : "\x1b[27m", encoder_states.enc_raw_a,
                                  encoder_states.enc_b ? "\x1b[7m" : "\x1b[27m", encoder_states.enc_raw_b,
                                  encoder_states.enc_z ? "\x1b[7m" : "\x1b[27m", encoder_states.enc_raw_z);
            }
        }

        /* Update the flap position. */
        flap_position_curr = flap_postion_get(&openflap_ctx);
        if (flap_position_curr != flap_position_prev) {
            flap_position_prev = flap_position_curr;
            debug_io_log_info("Flap Position: %d (%s)\n", flap_position_curr,
                              &openflap_ctx.config.symbol_set[flap_position_curr]);
        }

        /* Set debug pins based on flap position. */
        debug_pin_set(0, flap_postion_get(&openflap_ctx) & 1);
        debug_pin_set(1, flap_postion_get(&openflap_ctx) == 0);

        /* Control the motor. */
        // TODO

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