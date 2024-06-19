#include "openflap.h"

uint8_t pwmDutyCycleCalc(uint8_t distance)
{
    if (distance == 0) {
        return 0;
    }
    uint8_t min_pwm = 25;
    uint8_t max_pwm = 80;
    return (distance - 1) * (max_pwm - min_pwm) / (SYMBOL_CNT - 2) + min_pwm;
}

void encoderPositionUpdate(openflap_ctx_t *ctx, uint32_t *adc_data)
{
    static uint8_t old_position = SYMBOL_CNT;
    uint8_t encoder_graycode = 0;

    for (uint8_t i = 0; i < ENCODER_RESOLUTION; i++) {
        if (adc_data[IR_MAP[i]] > ctx->config.ir_limits[i]) {
            encoder_graycode &= ~(1 << i);
        } else if (adc_data[IR_MAP[i]] < ctx->config.ir_limits[i]) {
            encoder_graycode |= (1 << i);
        }
    }

    // Convert grey code into decimal.
    uint8_t encoder_decimal = 0;
    for (encoder_decimal = 0; encoder_graycode; encoder_graycode = encoder_graycode >> 1) {
        encoder_decimal ^= encoder_graycode;
    }
    // Reverse encoder direction.
    uint8_t new_position = (uint8_t)SYMBOL_CNT - encoder_decimal - 1;

    // Ignore erroneous reading.
    if (new_position < SYMBOL_CNT) {
        new_position = flapIndexWrapCalc(new_position + ctx->config.encoder_offset);
        // Ignore sensor backspin.
        if (flapIndexWrapCalc(new_position + 1) != old_position) {
            old_position = new_position;
            ctx->flap_position = new_position;
        }
    }
}

uint8_t getAdcBasedRandSeed(uint32_t *adc_data)
{
    /* Use ADC noise to generate a random number. */
    uint8_t rand_seed = 0;
    for (uint8_t i = 0; i < ENCODER_RESOLUTION; i++) {
        rand_seed <<= 1;
        rand_seed ^= (adc_data[i] & 0x03);
    }
    return rand_seed;
}