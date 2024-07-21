#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef struct rbuff_tag {
    uint8_t r_cnt;
    uint8_t w_cnt;
    uint8_t *buff;
    uint8_t size;
} rbuff_t;

/**
 * \brief Initialize a ringbuffer.
 *
 * \param rbuff The ringbuffer.
 * \param buff The raw array to be used by the ringbuffer.
 * \param size The size of the raw array \p buff.
 */
void rbuff_init(rbuff_t *rbuff, uint8_t *buff, uint8_t size);

/**
 * \brief Read one byte from the ringbuffer.
 *
 * \param rbuff The ringbuffer.
 * \param data The data read from the ringbuffer.
 * \param size The number of bytes to write.
 *
 * \return The number of bytes read.
 */
uint8_t rbuff_read(rbuff_t *rbuff, uint8_t *data, uint8_t size);

/**
 * \brief Write one byte to the ringbuffer.
 *
 * \param rbuff The ringbuffer.
 * \param data The data to be written to the ringbuffer.
 * \param size The number of bytes to write.
 *
 * \return The number of bytes written.
 */
uint8_t rbuff_write(rbuff_t *rbuff, uint8_t *data, uint8_t size);

/**
 * \brief Check if the ringbuffer is empty.
 *
 * \param rbuff The ringbuffer.
 * \return True if the ringbuffer is empty, otherwise false.
 */
bool rbuff_is_empty(rbuff_t *rbuff);

/**
 * \brief Check if the ringbuffer is full.
 *
 * \param rbuff The ringbuffer.
 * \return True if the ringbuffer is full, otherwise false.
 */
bool rbuff_is_full(rbuff_t *rbuff);

/**
 * \brief Get the number of used elements in the ringbuffer.
 *
 * \param rbuff The ringbuffer.
 * \return The number of used elements in the ringbuffer.
 */
uint8_t rbuff_cnt_used(rbuff_t *rbuff);

/**
 * \brief Get the number of free elements in the ringbuffer.
 *
 * \param rbuff The ringbuffer.
 * \return The number of free elements in the ringbuffer.
 */
uint8_t rbuff_cnt_free(rbuff_t *rbuff);

/**
 * \brief Flush the ringbuffer.
 *
 * \param rbuff The ringbuffer.
 */
void rbuff_flush(rbuff_t *rbuff);

/**
 * \brief Peek at the next byte in the ringbuffer.
 *
 * \param rbuff The ringbuffer.
 * \param data The data peeked from the ringbuffer.
 * \param size The number of bytes to peek.
 *
 * \return The number of bytes peeked.
 */
uint8_t rbuff_peek(rbuff_t *rbuff, uint8_t *data, uint8_t size);
