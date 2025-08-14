#include "rbuff.h"

#include "unity.h"

#include <string.h>

/* Exposing private functions. */
extern void *rbuff_r_ptr_next(rbuff_t *rbuff);
extern void *rbuff_w_ptr_current(rbuff_t *rbuff);
extern void *rbuff_w_ptr_next(rbuff_t *rbuff);

/* Function and global variable for testing DMA read pointer. */
void *get_read_pointer(void);
static void *dma_read_pointer;

//----------------------------------------------------------------------------------------------------------------------

void setUp(void)
{
    // This function is called before each test
}

void tearDown(void)
{
    // This function is called after each test
}

//----------------------------------------------------------------------------------------------------------------------

void test_rbuff_init(void)
{
    uint8_t buffer[10];
    rbuff_t rbuff;

    rbuff_init(&rbuff, buffer, 10, sizeof(buffer[0]));

    TEST_ASSERT_EQUAL(buffer, rbuff.buff);
    TEST_ASSERT_EQUAL(10, rbuff.capacity);
    TEST_ASSERT_EQUAL(sizeof(uint8_t), rbuff.element_size);
    TEST_ASSERT_EQUAL(buffer, rbuff.w_ptr);
    TEST_ASSERT_EQUAL(buffer, rbuff.r_ptr);
    TEST_ASSERT_EQUAL(buffer + 10, rbuff.buff_end);
    TEST_ASSERT_FALSE(rbuff.dma_ro);
}

//----------------------------------------------------------------------------------------------------------------------

void test_rbuff_init_dma_ro(void)
{
    uint8_t buffer[10];
    rbuff_t rbuff;

    rbuff_init_dma_ro(&rbuff, buffer, 10, sizeof(buffer[0]), (dma_rw_ptr_get_cb)0xdeadbeef);

    TEST_ASSERT_EQUAL(buffer, rbuff.buff);
    TEST_ASSERT_EQUAL(10, rbuff.capacity);
    TEST_ASSERT_EQUAL(sizeof(uint8_t), rbuff.element_size);
    TEST_ASSERT_EQUAL(buffer, rbuff.w_ptr);
    TEST_ASSERT_EQUAL(buffer, rbuff.r_ptr);
    TEST_ASSERT_EQUAL(buffer + 10, rbuff.buff_end);
    TEST_ASSERT_TRUE(rbuff.dma_ro);
    TEST_ASSERT_EQUAL((dma_rw_ptr_get_cb)0xdeadbeef, rbuff.dma_rw_ptr_get);
}

//----------------------------------------------------------------------------------------------------------------------

void test_rbuff_read(void)
{
    uint8_t buffer[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    rbuff_t rbuff;
    uint8_t read_data[10];

    rbuff_init(&rbuff, buffer, 10, sizeof(buffer[0]));

    /* Try to read 5 bytes, but none have been written. */
    memset(read_data, 0, sizeof(read_data));
    TEST_ASSERT_EQUAL(0, rbuff_read(&rbuff, read_data, 5));
    TEST_ASSERT_EQUAL_UINT8_ARRAY(((uint8_t[]) {0, 0, 0, 0, 0}), read_data, 5);
    TEST_ASSERT_EQUAL(buffer, rbuff.r_ptr);

    /* Try to read 10 bytes, but only 5 have been written. */
    memset(read_data, 0, sizeof(read_data));
    rbuff.w_ptr += 5;
    TEST_ASSERT_EQUAL(5, rbuff_read(&rbuff, read_data, 10));
    TEST_ASSERT_EQUAL_UINT8_ARRAY(((uint8_t[]) {0, 1, 2, 3, 4}), read_data, 5);
    TEST_ASSERT_EQUAL(buffer + 5, rbuff.r_ptr);
}

//----------------------------------------------------------------------------------------------------------------------

void test_rbuff_write(void)
{
    uint8_t buffer[10] = {0};
    rbuff_t rbuff;

    rbuff_init(&rbuff, buffer, 10, sizeof(buffer[0]));

    /* Write 5 bytes. */
    TEST_ASSERT_EQUAL(5, rbuff_write(&rbuff, ((uint8_t[]) {0, 1, 2, 3, 4}), 5));
    TEST_ASSERT_EQUAL_UINT8_ARRAY(((uint8_t[]) {0, 1, 2, 3, 4, 0, 0, 0, 0, 0}), buffer, 10);
    TEST_ASSERT_EQUAL(buffer + 5, rbuff.w_ptr);

    /* Try to write 10 more but we can only fit 4 more */
    TEST_ASSERT_EQUAL(4, rbuff_write(&rbuff, ((uint8_t[]) {0, 1, 2, 3, 4, 5, 6, 7, 8, 9}), 10));
    TEST_ASSERT_EQUAL_UINT8_ARRAY(((uint8_t[]) {0, 1, 2, 3, 4, 0, 1, 2, 3}), buffer, 9);
    TEST_ASSERT_EQUAL(buffer + 9, rbuff.w_ptr);
}

//----------------------------------------------------------------------------------------------------------------------

void test_rbuff_is_empty(void)
{
    uint8_t buffer[10] = {0};
    rbuff_t rbuff;

    rbuff_init(&rbuff, buffer, 10, sizeof(buffer[0]));

    /* Initially empty. */
    rbuff.r_ptr = buffer + 0;
    rbuff.w_ptr = buffer + 0;
    TEST_ASSERT_TRUE(rbuff_is_empty(&rbuff));

    /* Stuff has been written. */
    rbuff.r_ptr = buffer + 5;
    rbuff.w_ptr = buffer + 0;
    TEST_ASSERT_FALSE(rbuff_is_empty(&rbuff));

    /* Stuff has been read. */
    rbuff.r_ptr = buffer + 0;
    rbuff.w_ptr = buffer + 5;
    TEST_ASSERT_FALSE(rbuff_is_empty(&rbuff));

    /* Equal amount of Stuff has been written and read. */
    rbuff.r_ptr = buffer + 5;
    rbuff.w_ptr = buffer + 5;
    TEST_ASSERT_TRUE(rbuff_is_empty(&rbuff));
}

//----------------------------------------------------------------------------------------------------------------------

void test_rbuff_is_full(void)
{
    uint8_t buffer[10] = {0};
    rbuff_t rbuff;

    rbuff_init(&rbuff, buffer, 10, sizeof(buffer[0]));

    /* Initially empty. */
    rbuff.r_ptr = buffer + 0;
    rbuff.w_ptr = buffer + 0;
    TEST_ASSERT_FALSE(rbuff_is_full(&rbuff));

    /* Some stuff has been written. */
    rbuff.r_ptr = buffer + 0;
    rbuff.w_ptr = buffer + 5;
    TEST_ASSERT_FALSE(rbuff_is_full(&rbuff));

    /* Stuff been written till full has been written. */
    rbuff.r_ptr = buffer + 0;
    rbuff.w_ptr = buffer + 9;
    TEST_ASSERT_TRUE(rbuff_is_full(&rbuff));
}

//----------------------------------------------------------------------------------------------------------------------

void test_rbuff_cnt_used(void)
{
    uint8_t buffer[10] = {0};
    rbuff_t rbuff;

    rbuff_init(&rbuff, buffer, 10, sizeof(buffer[0]));

    /* Initially empty. */
    rbuff.r_ptr = buffer + 0;
    rbuff.w_ptr = buffer + 0;
    TEST_ASSERT_EQUAL(0, rbuff_cnt_used(&rbuff));

    /* Some stuff has been written. */
    rbuff.r_ptr = buffer + 0;
    rbuff.w_ptr = buffer + 4;
    TEST_ASSERT_EQUAL(4, rbuff_cnt_used(&rbuff));

    /* Some stuff has been written and read. */
    rbuff.r_ptr = buffer + 2;
    rbuff.w_ptr = buffer + 5;
    TEST_ASSERT_EQUAL(3, rbuff_cnt_used(&rbuff));

    /* Case where ring buffer wraps around. */
    rbuff.r_ptr = buffer + 8;
    rbuff.w_ptr = buffer + 5;
    TEST_ASSERT_EQUAL(7, rbuff_cnt_used(&rbuff));
}

//----------------------------------------------------------------------------------------------------------------------

void test_rbuff_cnt_free(void)
{
    uint8_t buffer[10] = {0};
    rbuff_t rbuff;

    rbuff_init(&rbuff, buffer, 10, sizeof(buffer[0]));

    /* Initially empty. */
    rbuff.r_ptr = buffer + 0;
    rbuff.w_ptr = buffer + 0;
    TEST_ASSERT_EQUAL(9, rbuff_cnt_free(&rbuff));

    /* Some stuff has been written. */
    rbuff.r_ptr = buffer + 0;
    rbuff.w_ptr = buffer + 4;
    TEST_ASSERT_EQUAL(5, rbuff_cnt_free(&rbuff));

    /* Some stuff has been written and read. */
    rbuff.r_ptr = buffer + 2;
    rbuff.w_ptr = buffer + 5;
    TEST_ASSERT_EQUAL(6, rbuff_cnt_free(&rbuff));

    /* Case where ring buffer wraps around. */
    rbuff.r_ptr = buffer + 8;
    rbuff.w_ptr = buffer + 5;
    TEST_ASSERT_EQUAL(2, rbuff_cnt_free(&rbuff));
}

//----------------------------------------------------------------------------------------------------------------------

void test_rbuff_flush(void)
{
    uint8_t buffer[10] = {0};
    rbuff_t rbuff;

    rbuff_init(&rbuff, buffer, 10, sizeof(buffer[0]));

    /* Initially empty. */
    rbuff.r_ptr = buffer + 0;
    rbuff.w_ptr = buffer + 5;
    TEST_ASSERT_FALSE(rbuff_is_empty(&rbuff));

    /* Flush to make it empty. */
    rbuff_flush(&rbuff);
    TEST_ASSERT_TRUE(rbuff_is_empty(&rbuff));
}

//----------------------------------------------------------------------------------------------------------------------

void test_rbuff_peek(void)
{
    uint8_t buffer[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    rbuff_t rbuff;
    uint8_t peek_data[10];

    rbuff_init(&rbuff, buffer, 10, sizeof(buffer[0]));

    /* Try to peek 10 bytes, but only 5 have been written. */
    memset(peek_data, 0, sizeof(peek_data));
    rbuff.w_ptr += 5;
    TEST_ASSERT_EQUAL(5, rbuff_peek(&rbuff, peek_data, 10));
    TEST_ASSERT_EQUAL_UINT8_ARRAY(((uint8_t[]) {0, 1, 2, 3, 4}), peek_data, 5);
    TEST_ASSERT_EQUAL(buffer, rbuff.r_ptr); /* Read pointer still unchanged. */
}

//----------------------------------------------------------------------------------------------------------------------

void test_rbuff_r_ptr_next(void)
{
    uint8_t buffer[10] = {0};
    rbuff_t rbuff;

    rbuff_init(&rbuff, buffer, 10, sizeof(buffer[0]));

    /* Initially empty. */
    rbuff.r_ptr = buffer + 0;
    TEST_ASSERT_EQUAL(buffer + 1, rbuff_r_ptr_next(&rbuff));

    /* Wrap around case. */
    rbuff.r_ptr = buffer + 9;
    TEST_ASSERT_EQUAL(buffer + 0, rbuff_r_ptr_next(&rbuff));
}

//----------------------------------------------------------------------------------------------------------------------

void test_rbuff_w_ptr_current(void)
{
    uint8_t buffer[10] = {0};
    rbuff_t rbuff;

    /* Normal Ring buffer. */
    rbuff_init(&rbuff, buffer, 10, sizeof(buffer[0]));

    /* Initially empty. */
    rbuff.w_ptr = buffer + 0;
    TEST_ASSERT_EQUAL(buffer + 0, rbuff_w_ptr_current(&rbuff));

    /* After writing some data. */
    rbuff.w_ptr = buffer + 5;
    TEST_ASSERT_EQUAL(buffer + 5, rbuff_w_ptr_current(&rbuff));

    /* DMA based Ring buffer. */
    rbuff_init_dma_ro(&rbuff, buffer, 10, sizeof(buffer[0]), get_read_pointer);
    dma_read_pointer = buffer + 5;
    TEST_ASSERT_EQUAL(buffer + 5, rbuff_w_ptr_current(&rbuff));
}

//----------------------------------------------------------------------------------------------------------------------

void test_rbuff_w_ptr_next(void)
{
    uint8_t buffer[10] = {0};
    rbuff_t rbuff;

    rbuff_init(&rbuff, buffer, 10, sizeof(buffer[0]));

    /* Initially empty. */
    rbuff.w_ptr = buffer + 0;
    TEST_ASSERT_EQUAL(buffer + 1, rbuff_w_ptr_next(&rbuff));

    /* Wrap around case. */
    rbuff.w_ptr = buffer + 9;
    TEST_ASSERT_EQUAL(buffer + 0, rbuff_w_ptr_next(&rbuff));
}

//======================================================================================================================
//                                                         PRIVATE FUNCTIONS
//======================================================================================================================

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_rbuff_init);
    RUN_TEST(test_rbuff_init_dma_ro);
    RUN_TEST(test_rbuff_read);
    RUN_TEST(test_rbuff_write);
    RUN_TEST(test_rbuff_is_empty);
    RUN_TEST(test_rbuff_is_full);
    RUN_TEST(test_rbuff_cnt_used);
    RUN_TEST(test_rbuff_cnt_free);
    RUN_TEST(test_rbuff_flush);
    RUN_TEST(test_rbuff_peek);
    RUN_TEST(test_rbuff_r_ptr_next);
    RUN_TEST(test_rbuff_w_ptr_current);
    RUN_TEST(test_rbuff_w_ptr_next);

    return UNITY_END();
}

//----------------------------------------------------------------------------------------------------------------------

void *get_read_pointer(void)
{
    return dma_read_pointer;
}