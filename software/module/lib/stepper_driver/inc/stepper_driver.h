#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct stepper_driver_ctx_tag stepper_driver_ctx_t;

typedef void (*stepper_driver_set_pins_cb_t)(stepper_driver_ctx_t *ctx, bool a_p, bool a_n, bool b_p, bool b_n);
typedef uint32_t (*stepper_driver_dynamic_speed_cb_t)(stepper_driver_ctx_t *ctx, uint32_t step_cnt,
                                                      uint32_t step_cnt_max);

typedef enum {
    STEPPER_DRIVER_DIR_CW  = 0, /**< Clockwise direction. */
    STEPPER_DRIVER_DIR_CCW = 1, /**< Counter-clockwise direction. */
} stepper_driver_dir_t;

typedef enum {
    STEPPER_DRIVER_MODE_WAVE_DRIVE = 0, /**< Wave drive mode. */
    STEPPER_DRIVER_MODE_FULL_DRIVE = 1, /**< Full drive mode. */
    STEPPER_DRIVER_MODE_HALF_DRIVE = 2, /**< Half drive mode. */
} stepper_driver_mode_t;

#define STEPPER_DRIVER_STEP_INFINITE 0xFFFFFFFF /**< Infinite degrees. */
#define STEPPER_DRIVER_SPEED_DYNAMIC 0xFFFF     /**< Maximum degrees (360 degrees * 10). */

typedef struct stepper_driver_ctx_tag {
    uint32_t tick_period_us;                    /**< The tick period in microseconds. */
    uint32_t steps_per_revolution;              /**< The number of steps per revolution. */
    stepper_driver_set_pins_cb_t set_pins_cb;   /**< Callback for seting the output pins. */
    stepper_driver_dynamic_speed_cb_t speed_cb; /**< Callback for dynamic speed. */
    stepper_driver_dir_t direction;             /**< The direction of the stepper motor. */
    int8_t mode_sequence_idx;                   /**< The index of the current mode sequence. */
    stepper_driver_mode_t mode;                 /**< The drive mode of the stepper motor. */
    const bool (*mode_sequence)[4];             /**< The drive mode sequence. */
    uint8_t mode_sequence_size;                 /**< The size of the drive mode sequence. */
    bool use_dynamic_speed;                     /**< Flag to use dynamic speed. */
    uint32_t rps_x100;                          /**< The speed in rotations per tenth of a second. */
    uint32_t ticks_per_step;                    /**< The number of ticks per step. */
    uint32_t ticks_cnt;                         /**< The tick counter. */
    uint32_t step_cnt;                          /**< The step count. */
    uint32_t step_cnt_target;                   /**< The maximum step count. */
    uint32_t virtual_steps_per_revolution; /**< The driver will act as if the stepper only has this amount of steps. */
    uint32_t virtual_step_offset;          /**< The offset of the virtual steps. */
    uint32_t full_rotations_todo;          /**< The number of full rotations todo. */
} stepper_driver_ctx_t;

/**
 * \brief Initialize the stepper driver.
 *
 * \param[inout] ctx The stepper driver context.
 * \param[in] tick_period_us The tick period in microseconds.
 * \param[in] steps_per_revolution The number of steps per revolution of the stepper motor.
 * \param[in] virtual_steps_per_revolution When set with a non zero value, the driver will act as if the stepper only
 * has this amount of steps. This value must be at least twice as small as the steps_per_revolution.
 * \param[in] set_pins_cb The callback for setting the output pins.
 * \param[in] speed_cb The callback for dynamic speed, may be NULL if static speed is used.
 */
void stepper_driver_init(stepper_driver_ctx_t *ctx, uint32_t tick_period_us, uint32_t steps_per_revolution,
                         uint32_t virtual_steps_per_revolution, stepper_driver_set_pins_cb_t set_pins_cb,
                         stepper_driver_dynamic_speed_cb_t speed_cb);

/**
 * \brief Stepper driver tick function.
 *
 * This function should be called periodically in a timer interrupt according to the tick period set during
 * initialization.
 *
 * \param[in] ctx The stepper driver context.
 *
 * \return true if driver has completed a step and started a new one, false if a step is incomplete.
 */
bool stepper_driver_tick(stepper_driver_ctx_t *ctx);

/**
 * \brief Set the stepper driver speed.
 *
 * \param[in] ctx The stepper driver context.
 * \param[in] rps_x100 The speed in revolutions per second multiplied by 10. Use STEPPER_DRIVER_SPEED_DYNAMIC to have
 * the speed dynamically changed after each step by calling the speed callback function.
 */
void stepper_driver_speed_set(stepper_driver_ctx_t *ctx, uint16_t rps_x100);

/**
 * \brief Set the stepper driver direction.
 *
 * \param[in] ctx The stepper driver context.
 * \param[in] dir The direction of the stepper motor.
 */
void stepper_driver_dir_set(stepper_driver_ctx_t *ctx, stepper_driver_dir_t dir);

/**
 * \brief Set the stepper driver mode.
 *
 * \param[in] ctx The stepper driver context.
 * \param[in] mode The drive mode of the stepper motor.
 */
void stepper_driver_mode_set(stepper_driver_ctx_t *ctx, stepper_driver_mode_t mode);

/**
 * \brief Rotate the stepper motor a certain number of degrees.
 *
 * \param[in] ctx The stepper driver context.
 * \param[in] degrees_x10 The number of degrees multiplied by 10. Use STEPPER_DRIVER_STEP_INFINITE to rotate
 *                        indefinitely.
 */
void stepper_driver_rotate(stepper_driver_ctx_t *ctx, uint32_t degrees_x10);

/**
 * \brief Set the offset for the virtual steps.
 *
 * \param[in] ctx The stepper driver context.
 * \param[in] offset The offset for the virtual steps.
 */
void stepper_driver_virtual_step_offset_set(stepper_driver_ctx_t *ctx, uint32_t offset);

/**
 * \brief Get the current position of the stepper motor.
 *
 * \param[in] ctx The stepper driver context.
 *
 * \note If the virtual steps are set, the position will be in virtual steps.
 *
 * \return The current position of the stepper motor in steps.
 */
uint32_t stepper_driver_position_get(stepper_driver_ctx_t *ctx);

/**
 * \brief Set the current position of the stepper motor.
 *
 * \param[in] ctx The stepper driver context.
 * \param[in] position The position to set in steps.
 *
 * \note If the virtual steps are set, the position will be in virtual steps.
 */
void stepper_driver_position_set(stepper_driver_ctx_t *ctx, uint32_t position);

/**
 * \brief Indicate that the current position is the home position.
 */
void stepper_driver_home_set(stepper_driver_ctx_t *ctx);

/**
 * \brief Check if the motor is spinning.
 *
 * \param[in] ctx The stepper driver context.
 *
 * \return true if the motor is spinning, false otherwise.
 */
bool stepper_driver_is_spinning(stepper_driver_ctx_t *ctx);