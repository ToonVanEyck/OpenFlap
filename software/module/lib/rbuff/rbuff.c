#include "rbuff.h"

void rbuff_init(rbuff_t *rbuff, uint8_t *buff, uint8_t size)
{
    rbuff->buff = buff;
    rbuff->size = size;
    rbuff->r_cnt = 0;
    rbuff->w_cnt = 0;
}

uint8_t rbuff_read(rbuff_t *rbuff, uint8_t *data, uint8_t size)
{
    uint8_t cnt = 0;
    while (cnt < size && !rbuff_is_empty(rbuff)) {
        data[cnt++] = rbuff->buff[rbuff->r_cnt++];
        rbuff->r_cnt = rbuff->r_cnt % rbuff->size;
    }
    return cnt;
}

uint8_t rbuff_write(rbuff_t *rbuff, uint8_t *data, uint8_t size)
{
    uint8_t cnt = 0;
    while (cnt < size && !rbuff_is_full(rbuff)) {
        rbuff->buff[rbuff->w_cnt++] = data[cnt++];
        rbuff->w_cnt = rbuff->w_cnt % rbuff->size;
    }
    return cnt;
}

bool rbuff_is_empty(rbuff_t *rbuff)
{
    return (rbuff->r_cnt == rbuff->w_cnt);
}

bool rbuff_is_full(rbuff_t *rbuff)
{
    return (rbuff->r_cnt == (rbuff->w_cnt + 1) % rbuff->size);
}

uint8_t rbuff_cnt_used(rbuff_t *rbuff)
{
    return (rbuff->w_cnt - rbuff->r_cnt + rbuff->size) % rbuff->size;
}

uint8_t rbuff_cnt_free(rbuff_t *rbuff)
{
    return rbuff->size - rbuff_cnt_used(rbuff) - 1;
}

void rbuff_flush(rbuff_t *rbuff)
{
    rbuff->r_cnt = 0;
    rbuff->w_cnt = 0;
}

uint8_t rbuff_peek(rbuff_t *rbuff, uint8_t *data, uint8_t size)
{
    uint8_t cnt = 0, r_cnt = rbuff->r_cnt;
    while (cnt < size && !rbuff_is_empty(rbuff)) {
        data[cnt++] = rbuff->buff[r_cnt++];
        r_cnt = r_cnt % size;
    }
    return cnt;
}