/* Includes ------------------------------------------------------------------*/
// #include "chain_comm.h"
#include "debug_io.h"
#include "debug_term.h"
#include "interpolation.h"
#include "openflap.h"
#include "property_handlers.h"

#include <stdio.h>
#include <stdlib.h>

/* Private define ------------------------------------------------------------*/

#ifndef VERSION
#define VERSION "not found"
#endif

/* Type definitions ---------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

static of_ctx_t of_ctx = {0}; /** The OpenFlap context. */

extern uint32_t checksum; /**< The checksum of the firmware, used for versioning. */

/* Private macro -------------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

static void debug_term_cmd_toggle_debug_pin(const char *input, void *arg)
{
    (void)arg;

    int pin = atoi(input);
    of_hal_debug_pin_toggle(pin);
}

int main(void)
{
    /* Initialize the hardware abstraction layer. */
    of_hal_init(&of_ctx.of_hal);

    /* Load the configuration from flash. */
    of_hal_config_load(&of_ctx.of_config);

    /* Initialize debugging features.*/
    debug_term_init(&of_ctx);

    /* Initialize chain communication and property handlers. */
    chain_comm_init(&of_ctx.chain_ctx, &of_ctx.of_hal.uart_driver);
    property_handlers_init(&of_ctx);

    /* Print boot messages. */
    uint32_t checksum_be = ((checksum & 0x000000FF) << 24) | ((checksum & 0x0000FF00) << 8) |
                           ((checksum & 0x00FF0000) >> 8) | ((checksum & 0xFF000000) >> 24);
    debug_io_log_info("App Name : OpenFlap module\n");
    debug_io_log_info("Version  : %s \n", GIT_VERSION);
    debug_io_log_info("CRC      : %08x\n", checksum_be);

    /* Start homing sequence at maximum speed. */
    of_ctx.flap_position = 0; /* Invalid position. */
    of_ctx.flap_setpoint = 0; /* Setpoint zero means, go to the first character in the character map. */
    /* Speed is based on distance between setpoint and position. We initialize it with the maximum value. */
    of_ctx.flap_distance = ENCODER_PULSES_PER_REVOLUTION;

    of_ctx.motor_control_override = true; /* Don't turn motor at start, wait for unlock command. */

    uint32_t sens_tick_prev = 0, sens_tick_curr = 0;

    debug_io_term_register_keyword("d", debug_term_cmd_toggle_debug_pin, &of_ctx);

    while (1) {
        of_hal_debug_pin_toggle(0);
        /* Handle the RTT terminal. */
        debug_io_term_process();

        /* If the module is the last module in a column, a secondary uart TX will route this modules data back to the
         * module above it. Normally this data would come from the module below it. */
        // of_hal_uart_tx_pin_update(of_hal_is_column_end());

        /* Run chain comm. */
        chain_comm(&of_ctx.chain_ctx);

        /* Update the sense timer tick count. */
        sens_tick_curr = of_hal_sens_tick_count_get();
        if (sens_tick_curr != sens_tick_prev) {

            /* Update the previous sensor tick count. */
            sens_tick_prev = sens_tick_curr;
        }

        /* Communication status. */
        comms_state_update(&of_ctx);

        /* Idle logic. */
        if (!of_ctx.motor_active && !of_ctx.comms_active) {
            if (of_ctx.store_config) {
                of_ctx.store_config = false;
                of_hal_config_store(&of_ctx.of_config);
                debug_io_log_info("Config stored!\n");
            }
            if (of_ctx.reboot) {
                debug_io_log_info("Rebooting module!\n");
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