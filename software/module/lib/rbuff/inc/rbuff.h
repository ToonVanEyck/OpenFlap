#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef void *(*dma_rw_ptr_get_cb)(void);

typedef struct rbuff_tag {
    void *buff;                       /**< Pointer to the raw array used by the ringbuffer. */
    void *buffer_end;                 /**< Pointer to the end of the raw array used by the ringbuffer. */
    void *w_ptr;                      /**< Pointer to the write pointer in the ringbuffer. */
    void *r_ptr;                      /**< Pointer to the read pointer in the ringbuffer. */
    size_t capacity;                  /**< Number of elements that fit in the buffer. */
    size_t element_size;              /**< Size of each element in the ringbuffer. */
    bool dma_ro;                      /**< True if the ringbuffer is read-only and uses DMA. */
    dma_rw_ptr_get_cb dma_rw_ptr_get; /**< Callback to get the DMA read/write pointer. */
} rbuff_t;

/**
 * @brief Initialize a ringbuffer.
 *
 * @param rbuff The ringbuffer.
 * @param buff The raw array to be used by the ringbuffer.
 * @param capacity The capacity of the ringbuffer in number of elements.
 * @param element_size The size of each element in the ringbuffer.
 */
void rbuff_init(rbuff_t *rbuff, void *buff, size_t capacity, size_t element_size);

/**
 * @brief Initialize a DMA ringbuffer for read-only access.
 *
 * This function initializes a ringbuffer that can only be read from. A DMA buffer is used to store the data, and is
 * written to by the hardware
 *
 * @param rbuff The ringbuffer.
 * @param buff The raw array to be used by the ringbuffer.
 * @param capacity The capacity of the ringbuffer in number of elements.
 * @param element_size The size of each element in the ringbuffer.
 * @param dma_w_ptr_get Callback to get the DMA write pointer.
 */
void rbuff_init_dma_ro(rbuff_t *rbuff, volatile void *buff, size_t capacity, size_t element_size,
                       dma_rw_ptr_get_cb dma_w_ptr_get);

/**
 * @brief Read one byte from the ringbuffer.
 *
 * @param rbuff The ringbuffer.
 * @param data The data read from the ringbuffer.
 * @param size The number of elements to read.
 *
 * @return The number of elements read.
 */
size_t rbuff_read(rbuff_t *rbuff, void *data, size_t size);

/**
 * @brief Write one byte to the ringbuffer.
 *
 * @param rbuff The ringbuffer.
 * @param data The data to be written to the ringbuffer.
 * @param size The number of elements to write.
 *
 * @return The number of elements written.
 */
size_t rbuff_write(rbuff_t *rbuff, void *data, size_t size);

/**
 * @brief Check if the ringbuffer is empty.
 *
 * @param rbuff The ringbuffer.
 * @return True if the ringbuffer is empty, otherwise false.
 */
bool rbuff_is_empty(rbuff_t *rbuff);

/**
 * @brief Check if the ringbuffer is full.
 *
 * @param rbuff The ringbuffer.
 * @return True if the ringbuffer is full, otherwise false.
 */
bool rbuff_is_full(rbuff_t *rbuff);

/**
 * @brief Get the number of used elements in the ringbuffer.
 *
 * @param rbuff The ringbuffer.
 * @return The number of used elements in the ringbuffer.
 */
size_t rbuff_cnt_used(rbuff_t *rbuff);

/**
 * @brief Get the number of free elements in the ringbuffer.
 *
 * @param rbuff The ringbuffer.
 * @return The number of free elements in the ringbuffer.
 */
size_t rbuff_cnt_free(rbuff_t *rbuff);

/**
 * @brief Flush the ringbuffer.
 *
 * @param rbuff The ringbuffer.
 */
void rbuff_flush(rbuff_t *rbuff);

/**
 * @brief Peek at the next byte in the ringbuffer.
 *
 * @param rbuff The ringbuffer.
 * @param data The data peeked from the ringbuffer.
 * @param size The number of bytes to peek.
 *
 * @return The number of bytes peeked.
 */
size_t rbuff_peek(rbuff_t *rbuff, void *data, size_t size);
