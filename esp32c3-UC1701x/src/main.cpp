// 定义屏幕类型：UC1701 或 ST7789
#define SCREEN_TYPE UC1701
// #define SCREEN_TYPE ST7789

#if SCREEN_TYPE == UC1701
#include "UC1701.h" // Include the header file instead of the .cpp file
using namespace UC1701;
#elif SCREEN_TYPE == ST7789
#include "ST7789.h" // Include the header file instead of the .cpp file
using namespace ST7789;
#else
#error "Unsupported SCREEN_TYPE. Please define UC1701 or ST7789."
#endif

void setup() {
#if SCREEN_TYPE == UC1701
    UC1701::setup(); // Explicitly call setup from the UC1701 namespace
#elif SCREEN_TYPE == ST7789
    ST7789::setup(); // Explicitly call setup from the ST7789 namespace
#endif
}

void loop() {
#if SCREEN_TYPE == UC1701
    UC1701::loop(); // Explicitly call loop from the UC1701 namespace
#elif SCREEN_TYPE == ST7789
    ST7789::loop(); // Explicitly call loop from the ST7789 namespace
#endif
}