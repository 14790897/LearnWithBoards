#include <Arduino.h>
#include <unity.h>
#include <FastLED.h>

// 模拟WS2812效果的基本测试

#define NUM_LEDS 60
CRGB testLeds[NUM_LEDS];

void setUp(void) {
    // 每个测试前的初始化
    FastLED.addLeds<WS2812B, 18, GRB>(testLeds, NUM_LEDS);
    fill_solid(testLeds, NUM_LEDS, CRGB::Black);
}

void tearDown(void) {
    // 每个测试后的清理
}

// 测试LED数组初始化
void test_led_initialization(void) {
    // 检查LED数组是否正确初始化为黑色
    for (int i = 0; i < NUM_LEDS; i++) {
        TEST_ASSERT_EQUAL(0, testLeds[i].r);
        TEST_ASSERT_EQUAL(0, testLeds[i].g);
        TEST_ASSERT_EQUAL(0, testLeds[i].b);
    }
}

// 测试颜色设置
void test_color_setting(void) {
    // 设置第一个LED为红色
    testLeds[0] = CRGB::Red;
    
    TEST_ASSERT_EQUAL(255, testLeds[0].r);
    TEST_ASSERT_EQUAL(0, testLeds[0].g);
    TEST_ASSERT_EQUAL(0, testLeds[0].b);
}

// 测试HSV到RGB转换
void test_hsv_conversion(void) {
    CHSV hsv_color(0, 255, 255); // 红色HSV
    CRGB rgb_color = hsv_color;
    
    TEST_ASSERT_EQUAL(255, rgb_color.r);
    TEST_ASSERT_EQUAL(0, rgb_color.g);
    TEST_ASSERT_EQUAL(0, rgb_color.b);
}

// 测试数组填充
void test_fill_solid(void) {
    fill_solid(testLeds, NUM_LEDS, CRGB::Blue);
    
    for (int i = 0; i < NUM_LEDS; i++) {
        TEST_ASSERT_EQUAL(0, testLeds[i].r);
        TEST_ASSERT_EQUAL(0, testLeds[i].g);
        TEST_ASSERT_EQUAL(255, testLeds[i].b);
    }
}

// 测试fadeToBlackBy函数
void test_fade_to_black(void) {
    testLeds[0] = CRGB(100, 100, 100);
    testLeds[0].fadeToBlackBy(50); // 减少约20%亮度
    
    // 验证颜色确实变暗了
    TEST_ASSERT_LESS_THAN(100, testLeds[0].r);
    TEST_ASSERT_LESS_THAN(100, testLeds[0].g);
    TEST_ASSERT_LESS_THAN(100, testLeds[0].b);
}

// 测试数学计算 - 彩虹效果的色相计算
void test_rainbow_hue_calculation(void) {
    uint8_t hue = 0;
    uint8_t pixelHue = hue + (0 * 256 / NUM_LEDS);
    
    TEST_ASSERT_EQUAL(0, pixelHue);
    
    pixelHue = hue + (15 * 256 / NUM_LEDS); // 中间位置
    TEST_ASSERT_EQUAL(128, pixelHue);
}

void setup() {
    delay(2000); // 等待串口初始化
    
    UNITY_BEGIN();
    
    RUN_TEST(test_led_initialization);
    RUN_TEST(test_color_setting);
    RUN_TEST(test_hsv_conversion);
    RUN_TEST(test_fill_solid);
    RUN_TEST(test_fade_to_black);
    RUN_TEST(test_rainbow_hue_calculation);
    
    UNITY_END();
}

void loop() {
    // 测试完成后什么都不做
}
