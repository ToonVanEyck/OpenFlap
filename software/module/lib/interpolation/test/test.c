#include "interpolation.h"

#include "unity.h"

#include <string.h>

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

void test_linear_interpolation(void)
{
    int32_t input[]  = {0, 250, 500, 750, 1000};
    int32_t output[] = {0, -250, -500, -750, -1000};

    interp_ctx_t ctx;

    interpolation_linear_init(&ctx, input, 5, output, 5);

    /* Test regular cases. */
    TEST_ASSERT_EQUAL(-200, interpolation_linear_compute(&ctx, 200));
    TEST_ASSERT_EQUAL(-600, interpolation_linear_compute(&ctx, 600));

    /* Test cases where input matches exactly. */
    TEST_ASSERT_EQUAL(-250, interpolation_linear_compute(&ctx, 250));
    TEST_ASSERT_EQUAL(-750, interpolation_linear_compute(&ctx, 750));

    /* Test cases out of range. */
    TEST_ASSERT_EQUAL(0, interpolation_linear_compute(&ctx, -1));
    TEST_ASSERT_EQUAL(-1000, interpolation_linear_compute(&ctx, 1001));
}

//----------------------------------------------------------------------------------------------------------------------

void test_bilinear_interpolation(void)
{
    int32_t input_1[] = {0, 100, 300, 700, 1000};  /* non-uniform steps. */
    int32_t input_2[] = {-300, 0, 330, 870, 1200}; /* offset + uneven spacing. */

    /* output is a 5x5 grid row-major: f(x,y) = x/2 - y/3 + (x*y)/500 */
    int32_t output[] = {
        /* I1/I2   -300   0    330   870   1200 */
        /*   0   */ 100, 0,   -110, -290, -400,
        /* 100   */ 90,  50,  6,    -66,  -110,
        /* 300   */ 70,  150, 238,  382,  470,
        /* 700   */ 30,  350, 702,  1278, 1630,
        /* 1000  */ 0,   500, 1050, 1950, 2500,
    };

    interp_ctx_t ctx;
    interpolation_bilinear_init(&ctx, input_1, 5, input_2, 5, output, 25);

    /* Regular interpolation cases (non-grid points). */
    TEST_ASSERT_EQUAL(60, interpolation_bilinear_compute(&ctx, 50, -150));
    TEST_ASSERT_EQUAL(1540, interpolation_bilinear_compute(&ctx, 800, 900));

    /* Exact matches on grid. */
    TEST_ASSERT_EQUAL(0, interpolation_bilinear_compute(&ctx, 0, 0));
    TEST_ASSERT_EQUAL(238, interpolation_bilinear_compute(&ctx, 300, 330));

    /* Matches only on one axis. */
    TEST_ASSERT_EQUAL(310, interpolation_bilinear_compute(&ctx, 300, 600));
    TEST_ASSERT_EQUAL(1891, interpolation_bilinear_compute(&ctx, 790, 1200));

    /* Edge / out of range. */
    TEST_ASSERT_EQUAL(100, interpolation_bilinear_compute(&ctx, -10, -400));   // clamp lower left
    TEST_ASSERT_EQUAL(2500, interpolation_bilinear_compute(&ctx, 2000, 2000)); // clamp upper right
}

//======================================================================================================================
//                                                         PRIVATE FUNCTIONS
//======================================================================================================================

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_linear_interpolation);
    RUN_TEST(test_bilinear_interpolation);

    return UNITY_END();
}