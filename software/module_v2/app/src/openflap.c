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