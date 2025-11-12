/* Includes ------------------------------------------------------------------*/
#include "debug_term.h"
#include "interpolation.h"
#include "openflap.h"
#include "property_handlers.h"
#include "rtt_utils.h"

#include <stdio.h>

/* Private define ------------------------------------------------------------*/

#ifndef VERSION
#define VERSION "not found"
#endif

/* Type definitions ---------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

static of_ctx_t of_ctx = {0}; /** The OpenFlap context. */

extern uint32_t checksum; /**< The checksum of the firmware, used for versioning. */

/* Input and output tables for speed/decay to pwm interpolation. */
static const int32_t sdp_interpolation_speed_inputs[] = {0, 25, 30, 40, 50, 70, 100};
static const int32_t sdp_interpolation_decay_inputs[] = {0, 250, 500, 750, 1000};
static const int32_t sdp_interpolation_pwm_outputs[]  = {
    /* S/D    0    250  500  750  1000 */
    /*   0 */ 0,   0,   0,   0,   0,
    /*  25 */ 150, 210, 260, 300, 310,
    /*  30 */ 165, 230, 290, 340, 340,
    /*  40 */ 185, 280, 350, 412, 420,
    /*  50 */ 215, 325, 420, 490, 500,
    /*  70 */ 320, 500, 600, 675, 690,
    /* 100 */ 700, 800, 900, 900, 900,
};

/* Input and output tables for speed to decay interpolation. */
static const int32_t sd_interpolation_speed_inputs[]  = {30, 60};
static const int32_t sd_interpolation_decay_outputs[] = {650, 0};

/* Input and output tables for distance to speed interpolation. */
static const int32_t ds_interpolation_distance_inputs[] = {2, 6};
static const int32_t ds_interpolation_speed_outputs[]   = {18, 75};

/* Private macro -------------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

int main(void)
{
    /* Initialize the hardware abstraction layer. */
    of_hal_init(&of_ctx.of_hal);

    /* Load the configuration from flash. */
    of_hal_config_load(&of_ctx.of_config);

    /* Initialize debugging features.*/
    rtt_init();
    // rtt_scope_init("u4");
    debug_term_init(&of_ctx);

    /* Initialize madelink and property handlers. */
    mdl_node_uart_cb_cfg_t uart_cb = {.read          = (uart_read_cb_t)uart_driver_read,
                                      .cnt_readable  = (uart_cnt_readable_cb_t)uart_driver_cnt_readable,
                                      .write         = (uart_write_cb_t)uart_driver_write,
                                      .cnt_writable  = (uart_cnt_writable_cb_t)uart_driver_cnt_writable,
                                      .tx_buff_empty = (uart_tx_buff_empty_cb_t)uart_driver_tx_idle,
                                      .is_busy       = (uart_is_busy_cb_t)uart_driver_is_busy};

    mdl_node_init(&of_ctx.mdl_node_ctx, &uart_cb, &of_ctx.of_hal.uart_driver, mdl_prop_list, OF_MDL_PROP_CNT, &of_ctx);
    property_handlers_init(&of_ctx);

    /* Initialize the PID controller. */
    pid_init(&of_ctx.pid_ctx, 6000, 6, 0);
    pid_o_lim_update(&of_ctx.pid_ctx, -1000, 1000);
    pid_i_lim_update(&of_ctx.pid_ctx, -75, 75);

    interpolation_bilinear_init(&of_ctx.sdp_interpolation_ctx, sdp_interpolation_speed_inputs,
                                sizeof(sdp_interpolation_speed_inputs) / sizeof(sdp_interpolation_speed_inputs[0]),
                                sdp_interpolation_decay_inputs,
                                sizeof(sdp_interpolation_decay_inputs) / sizeof(sdp_interpolation_decay_inputs[0]),
                                sdp_interpolation_pwm_outputs,
                                sizeof(sdp_interpolation_pwm_outputs) / sizeof(sdp_interpolation_pwm_outputs[0]));

    interpolation_linear_init(&of_ctx.sd_interpolation_ctx, sd_interpolation_speed_inputs,
                              sizeof(sd_interpolation_speed_inputs) / sizeof(sd_interpolation_speed_inputs[0]),
                              sd_interpolation_decay_outputs,
                              sizeof(sd_interpolation_decay_outputs) / sizeof(sd_interpolation_decay_outputs[0]));

    interpolation_linear_init(&of_ctx.ds_interpolation_ctx, ds_interpolation_distance_inputs,
                              sizeof(ds_interpolation_distance_inputs) / sizeof(ds_interpolation_distance_inputs[0]),
                              ds_interpolation_speed_outputs,
                              sizeof(ds_interpolation_speed_outputs) / sizeof(ds_interpolation_speed_outputs[0]));

    /* Print boot messages. */
    uint32_t checksum_be = ((checksum & 0x000000FF) << 24) | ((checksum & 0x0000FF00) << 8) |
                           ((checksum & 0x00FF0000) >> 8) | ((checksum & 0xFF000000) >> 24);
    printf("App Name : OpenFlap module\n");
    printf("Version  : %s \n", GIT_VERSION);
    printf("CRC      : %08lx\n", checksum_be);

    /* Start homing sequence at maximum speed. */
    of_ctx.flap_position = 0; /* Invalid position. */
    of_ctx.flap_setpoint = 0; /* Setpoint zero means, go to the first character in the character map. */
    /* Speed is based on distance between setpoint and position. We initialize it with the maximum value. */
    of_ctx.flap_distance = ENCODER_PULSES_PER_REVOLUTION;

    of_ctx.motor_control_override = true; /* Don't turn motor at start, wait for unlock command. */

    uint32_t pwm_tick_prev = 0, pwm_tick_curr = 0;
    uint32_t sens_tick_prev = 0, sens_tick_curr = 0;

    while (1) {
        /* Handle the terminal input. */
        simple_term_process();

        /* If the module is the last module in a column, a secondary uart TX will route this modules data back to
         * the module above it. Normally this data would come from the module below it. */
        of_hal_uart_tx_pin_update(of_hal_is_column_end());

        /* Run madelink node. */
        mdl_node_tick(&of_ctx.mdl_node_ctx, of_hal_tick_count_get());

        /* Update the sense timer tick count. */
        sens_tick_curr = of_hal_sens_tick_count_get();
        if (sens_tick_curr != sens_tick_prev) {
            /* Calculate the new position, actual rotational speed and
             * desired rotational speed based on the distance left to travel. */
            of_encoder_values_update(&of_ctx);
            of_encoder_position_update(&of_ctx);
            of_encoder_speed_calc(&of_ctx, sens_tick_curr);
            of_speed_setpoint_set_from_distance(&of_ctx);

            /* Print ADC values once every 25 ticks if desired. */
            if (of_ctx.debug_flags.print_adc_values && sens_tick_curr % 25 == 0) {
                printf("raw: %s%04d\x1b[0m %s%04d\x1b[0m %s%04d\x1b[0m\n",
                       of_ctx.encoder.digital[ENC_CH_A] ? "\x1b[7m" : "\x1b[27m", of_ctx.encoder.analog[ENC_CH_A],
                       of_ctx.encoder.digital[ENC_CH_B] ? "\x1b[7m" : "\x1b[27m", of_ctx.encoder.analog[ENC_CH_B],
                       of_ctx.encoder.digital[ENC_CH_Z] ? "\x1b[7m" : "\x1b[27m", of_ctx.encoder.analog[ENC_CH_Z]);
            }

            /* Update the previous sensor tick count. */
            sens_tick_prev = sens_tick_curr;
        }

        /* Update the PWM timer tick count. */
        pwm_tick_curr = of_hal_pwm_tick_count_get();
        if (pwm_tick_curr != pwm_tick_prev) {
            pwm_tick_prev = pwm_tick_curr;

            motor_control_loop(&of_ctx, pwm_tick_curr);
        }

        /* Communication status. */
        comms_state_update(&of_ctx);

        /* Motor status. */
        motor_state_update(&of_ctx);

        /* Idle logic. */
        if (!of_ctx.motor_active && !of_ctx.comms_active) {
            if (of_ctx.store_config) {
                of_ctx.store_config = false;
                of_hal_config_store(&of_ctx.of_config);
                printf("Config stored!\n");
            }
            if (of_ctx.reboot) {
                printf("Rebooting module!\n");
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