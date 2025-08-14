#include "rbuff.h"

#include <assert.h>
#include <string.h>

//======================================================================================================================
//                                                  FUNCTION PROTOTYPES
//======================================================================================================================

void *rbuff_r_ptr_next(rbuff_t *rbuff);
void *rbuff_w_ptr_current(rbuff_t *rbuff);
void *rbuff_w_ptr_next(rbuff_t *rbuff);

//======================================================================================================================
//                                                   PUBLIC FUNCTIONS
//======================================================================================================================

void rbuff_init(rbuff_t *rbuff, void *buff, size_t capacity, size_t element_size)
{
    rbuff->buff         = buff;
    rbuff->capacity     = capacity;
    rbuff->element_size = element_size;
    rbuff->w_ptr        = buff;
    rbuff->r_ptr        = buff;
    rbuff->dma_ro       = false;
    rbuff->buff_end     = (char *)buff + capacity * element_size;
}

//----------------------------------------------------------------------------------------------------------------------

void rbuff_init_dma_ro(rbuff_t *rbuff, volatile void *buff, size_t capacity, size_t element_size,
                       dma_rw_ptr_get_cb dma_w_ptr_get)
{
    rbuff_init(rbuff, (void *)buff, capacity, element_size);
    rbuff->dma_ro         = true;
    rbuff->dma_rw_ptr_get = dma_w_ptr_get;
}

//----------------------------------------------------------------------------------------------------------------------

size_t rbuff_read(rbuff_t *rbuff, void *data, size_t size)
{
    size_t cnt = 0;
    while (cnt < size && !rbuff_is_empty(rbuff)) {
        memcpy((char *)data + (cnt * rbuff->element_size), rbuff->r_ptr, rbuff->element_size);
        rbuff->r_ptr = rbuff_r_ptr_next(rbuff);
        cnt++;
    }
    return cnt;
}

//----------------------------------------------------------------------------------------------------------------------

size_t rbuff_write(rbuff_t *rbuff, void *data, size_t size)
{
    assert(!rbuff->dma_ro);
    size_t cnt = 0;
    while (cnt < size && !rbuff_is_full(rbuff)) {
        memcpy(rbuff->w_ptr, (char *)data + (cnt * rbuff->element_size), rbuff->element_size);
        rbuff->w_ptr = rbuff_w_ptr_next(rbuff);
        cnt++;
    }
    return cnt;
}

//----------------------------------------------------------------------------------------------------------------------

bool rbuff_is_empty(rbuff_t *rbuff)
{
    volatile void *current_w_ptr = rbuff_w_ptr_current(rbuff);
    bool val                     = (rbuff->r_ptr == current_w_ptr);
    return val;
}

//----------------------------------------------------------------------------------------------------------------------

bool rbuff_is_full(rbuff_t *rbuff)
{
    return (rbuff->r_ptr == rbuff_w_ptr_next(rbuff));
}

//----------------------------------------------------------------------------------------------------------------------

size_t rbuff_cnt_used(rbuff_t *rbuff)
{
    return ((char *)rbuff_w_ptr_current(rbuff) - (char *)rbuff->r_ptr + (rbuff->capacity * rbuff->element_size)) %
           (rbuff->capacity * rbuff->element_size) / rbuff->element_size;
}

//----------------------------------------------------------------------------------------------------------------------

size_t rbuff_cnt_free(rbuff_t *rbuff)
{
    return rbuff->capacity - rbuff_cnt_used(rbuff) - 1;
}

//----------------------------------------------------------------------------------------------------------------------

void rbuff_flush(rbuff_t *rbuff)
{
    rbuff->r_ptr = rbuff_w_ptr_current(rbuff);
}

//----------------------------------------------------------------------------------------------------------------------

size_t rbuff_peek(rbuff_t *rbuff, void *data, size_t size)
{
    size_t cnt           = 0;
    void *original_r_ptr = rbuff->r_ptr;
    while (cnt < size && !rbuff_is_empty(rbuff)) {
        memcpy((char *)data + (cnt * rbuff->element_size), rbuff->r_ptr, rbuff->element_size);
        rbuff->r_ptr = rbuff_r_ptr_next(rbuff);
        cnt++;
    }
    rbuff->r_ptr = original_r_ptr; // Restore the read pointer to its original position
    return cnt;
}

//======================================================================================================================
//                                                         PRIVATE FUNCTIONS
//======================================================================================================================

/**
 * @brief Get the next read pointer in the ringbuffer.
 *
 * @param rbuff The ringbuffer.
 * @return Pointer to the next read position.
 */
void *rbuff_r_ptr_next(rbuff_t *rbuff)
{
    void *new_r_ptr = (char *)rbuff->r_ptr + rbuff->element_size;
    if (new_r_ptr == rbuff->buff_end) {
        new_r_ptr = rbuff->buff;
    }
    return new_r_ptr;
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * @brief Get the current write pointer in the ringbuffer.
 *
 * @param rbuff The ringbuffer.
 * @return Pointer to the current write position.
 */
void *rbuff_w_ptr_current(rbuff_t *rbuff)
{
    if (rbuff->dma_ro) {
        return rbuff->dma_rw_ptr_get();
    } else {
        return rbuff->w_ptr;
    }
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * @brief Get the next write pointer in the ringbuffer.
 *
 * @param rbuff The ringbuffer.
 * @return Pointer to the next write position.
 */
void *rbuff_w_ptr_next(rbuff_t *rbuff)
{
    void *new_w_ptr = (char *)rbuff_w_ptr_current(rbuff) + rbuff->element_size;
    if (new_w_ptr == rbuff->buff_end) {
        new_w_ptr = rbuff->buff;
    }
    return new_w_ptr;
}