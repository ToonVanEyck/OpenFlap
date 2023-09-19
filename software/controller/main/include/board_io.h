#ifndef BOARD_IO_H
#define BOARD_IO_H

#include "driver/gpio.h"
#include "driver/ledc.h"

#define FLAP_ENABLE_PIN GPIO_NUM_2
#define MODE_BTN_PIN GPIO_NUM_33

#define LEDC_HS_TIMER LEDC_TIMER_0
#define LEDC_HS_MODE LEDC_LOW_SPEED_MODE
#define LEDC_RED_GPIO GPIO_NUM_32
#define LEDC_RED_CHANNEL LEDC_CHANNEL_0
#define LEDC_GREEN_GPIO GPIO_NUM_12
#define LEDC_GREEN_CHANNEL LEDC_CHANNEL_1
#define LEDC_BLUE_GPIO GPIO_NUM_14
#define LEDC_BLUE_CHANNEL LEDC_CHANNEL_2
#define LEDC_RGB_CH_NUM (3)
#define LEDC_RGB_DUTY (4000)
#define LEDC_RGB_FADE_TIME (3000)

void board_io_init();

void setLed(uint8_t red, uint8_t green, uint8_t blue);

#endif