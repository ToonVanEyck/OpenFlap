#pragma once

#include "chain_comm.h"
#include "config.h"
#include "flash.h"
#include "platform.h"

/** Struct with helper variables. */
typedef struct openflap_ctx_tag {
    uint8_t flap_position;      /**< The current position of flap wheel. */
    uint8_t flap_setpoint;      /**< The desired position of flap wheel. */
    openflap_config_t config;   /**< The configuration data. */
    chain_comm_ctx_t chain_ctx; /**< The chain communication context. */
    bool store_config;          /**< Flag to store the configuration. */
    bool reboot;                /**< Flag to indicate the module must preform a system reboot. */
    bool is_idle;               /**< Flag to indicate if the flap wheel is idle. */
    uint32_t idle_start_ms;     /**< The time when the flap wheel started idling. */
    uint16_t ir_tick_cnt;       /**< Counter for determining IR sensor state. */
    uint32_t timeout_tick_cnt;  /**< Counter for determining timeout. */
} openflap_ctx_t;

/**
 * \brief Calculate a PWM duty cycle based on the distance between the setpoint and the encoder position.
 *
 * \param[in] distance The distance between the setpoint and the encoder position.
 * \return The calculated PWM duty cycle.
 */
uint8_t pwmDutyCycleCalc(uint8_t distance);

/**
 * \brief Map the index to a range between 0 and #SYMBOL_CNT.
 *
 * \param index the index.
 * \return The wrapped index.
 */
inline uint8_t flapIndexWrapCalc(int8_t index)
{
    return (((index % SYMBOL_CNT) + SYMBOL_CNT) % SYMBOL_CNT);
}

/**
 * \brief Calculate the encoder position based on the ADC data. The updated position is stored in the context.
 *
 * \param[in] ctx A pointer to the openflap context.
 * \param[in] adc_data The ADC data of the IR sensors.
 */
void encoderPositionUpdate(openflap_ctx_t *ctx, uint32_t *adc_data);

/**
 * \brief Generate a random seed based on the ADC data.
 *
 * \param[in] adc_data The ADC data of the IR sensors.
 *
 * \return The generated random seed.
 */
uint8_t getAdcBasedRandSeed(uint32_t *adc_data);