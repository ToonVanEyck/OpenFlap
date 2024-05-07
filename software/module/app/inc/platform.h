#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "debug_io.h"
#include "py32f0xx_bsp_clock.h"
#include "py32f0xx_hal.h"

/** Map sensor adc channel to bit in encoder byte:
 *  - Bit 0 <-- ADC channel 2
 *  - Bit 1 <-- ADC channel 3
 *  - Bit 2 <-- ADC channel 1
 *  - Bit 3 <-- ADC channel 4
 *  - Bit 4 <-- ADC channel 0
 *  - Bit 5 <-- ADC channel 5
 */
#define IR_MAP ((uint8_t[]){2, 3, 1, 4, 0, 5})

#define GPIO_PIN_LED GPIO_PIN_7
#define GPIO_PORT_LED GPIOA
#define GPIO_PIN_MOTOR GPIO_PIN_6
#define GPIO_PORT_MOTOR GPIOA
#define GPIO_PIN_COLEND GPIO_PIN_12
#define GPIO_PORT_COLEND GPIOA

/** Ring buffer type. */
typedef struct ring_buff_tag {
    uint8_t r_cnt;
    uint8_t w_cnt;
    uint8_t buf[16];
} ring_buf_t;

/**
 * \brief Check if how space of the ring buffer is used.
 */
inline bool rb_capacity_used(ring_buf_t *rb)
{
    return ((rb->w_cnt + sizeof(rb->buf)) - rb->r_cnt) & 0x0F;
}

/**
 * \brief Check if data is available in ring buffer.
 */
inline bool rb_data_available(ring_buf_t *rb)
{
    return (rb->r_cnt != rb->w_cnt);
}

/**
 * \brief Check if space is available in ring buffer.
 */
inline bool rb_space_available(ring_buf_t *rb)
{
    return ((rb->w_cnt + 1) & 0x0F) != rb->r_cnt;
}

/**
 * \brief Write data to ring buffer.
 */
inline void rb_data_enqueue(ring_buf_t *rb, uint8_t data)
{
    rb->buf[rb->w_cnt++] = data;
    rb->w_cnt &= 0x0F;
}

/**
 * \brief Read data from ring buffer.
 */
inline uint8_t rb_data_dequeue(ring_buf_t *rb)
{
    uint8_t data = rb->buf[rb->r_cnt++];
    rb->r_cnt &= 0x0F;
    return data;
}

/**
 * \brief Read data from ring buffer.
 */
inline uint8_t rb_data_peek(ring_buf_t *rb)
{
    uint8_t data = rb->buf[rb->r_cnt];
    return data;
}