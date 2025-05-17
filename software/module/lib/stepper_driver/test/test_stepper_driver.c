#include "stepper_driver.h"
#include "unity.h"

void setUp(void)
{
}
void tearDown(void)
{
}

void test_addition(void)
{
    TEST_ASSERT_EQUAL_INT(4, 2 + 2);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_addition);
    return UNITY_END();
}