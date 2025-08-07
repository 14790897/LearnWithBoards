// 定义屏幕类型：UC1701、ST7789 或 ILI9341
#define UC1701_number 0
#define ST7789_number 1
#define ILI9341_number 2
#define SCREEN_TYPE ST7789_number // Set to UC1701, ST7789 or ILI9341

#if SCREEN_TYPE == UC1701_number
#include "UC1701.h" // Include the header file instead of the .cpp file
using namespace UC1701;
#elif SCREEN_TYPE == ST7789_number
#include "ST7789.h" // Include the header file instead of the .cpp file
using namespace ST7789;
#elif SCREEN_TYPE == ILI9341_number
#include "ILI9341.h" // Include the header file instead of the .cpp file
using namespace ILI9341;
#else
#error "Unsupported SCREEN_TYPE. Please define UC1701, ST7789 or ILI9341."
#endif

void setup()
{
#if SCREEN_TYPE == UC1701_number
    UC1701::setup(); // Explicitly call setup from the UC1701 namespace
#elif SCREEN_TYPE == ST7789_number
    ST7789::setup(); // Explicitly call setup from the ST7789 namespace
#elif SCREEN_TYPE == ILI9341_number
    ILI9341::setup(); // Explicitly call setup from the ILI9341 namespace
#endif
}

void loop()
{
#if SCREEN_TYPE == UC1701_number
    UC1701::UC1701_loop(); // Explicitly call loop from the UC1701 namespace
#elif SCREEN_TYPE == ST7789_number
    ST7789::ST7789_loop(); // Explicitly call loop from the ST7789 namespace
#elif SCREEN_TYPE == ILI9341_number
    ILI9341::ILI9341_loop(); // Explicitly call loop from the ILI9341 namespace
#endif
}