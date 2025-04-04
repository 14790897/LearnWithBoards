#include <Arduino.h>
#include <unity.h>

void setUp(void) {
}

void tearDown(void) {
}

void test_hello_world(void) {
    TEST_ASSERT_EQUAL(1 + 1, 2);
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_hello_world);
    return UNITY_END();
}