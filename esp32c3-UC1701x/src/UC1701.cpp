#include <Arduino.h>
#include <U8g2lib.h>
#include "UC1701.h"
#include <SPI.h>

namespace UC1701 {

// 定义引脚
const uint8_t CLK_PIN = 2;  // SCK
const uint8_t MOSI_PIN = 3; // SDA/SI
const uint8_t CS_PIN = 7;   // ROM_CS
const uint8_t DC_PIN = 6;  // DC/RS
const uint8_t RES_PIN = 10;  // RES

// SPI 接口初始化
U8G2_UC1701_MINI12864_1_4W_HW_SPI u8g2(U8G2_R2, CS_PIN, DC_PIN, RES_PIN);

void setup()
{
    Serial.begin(115200);
    while (!Serial)
    {
        delay(10); // 等待串口
    }

    Serial.println("Starting setup...");

    // 初始化显示
    Serial.println("Initializing LCD...");
    if (!u8g2.begin())
    {
        Serial.println("LCD initialization failed! Check connections.");
        while (1)
        {
            delay(1000); // 停止程序，等待用户检查
        }
    }
    // delay(100); // 添加延迟以确保显示器稳定

    // 测试屏幕旋转方向
    u8g2.setDisplayRotation(U8G2_R0); // 尝试不同方向：U8G2_R0, U8G2_R1, U8G2_R2, U8G2_R3

    // u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_ncenB08_tr);
    u8g2.drawStr(120, 23, "ESP32-C3");
    u8g2.drawStr(110, 23, "UC1701x Test");
    u8g2.sendBuffer();

    Serial.println("LCD initialized");
    Serial.println("Setup complete.");
}

void loop()
{
    static int x = 0;       // X-coordinate of the square
    static int y = 20;      // Y-coordinate of the square
    static int dx = 2;      // X-direction increment
    static int dy = 1;      // Y-direction increment
    const int squareSize = 10; // Size of the square

    // Clear the buffer
    // u8g2.clearBuffer();

    // Draw the moving square
    u8g2.drawBox(x, y, squareSize, squareSize);

    // Update the square's position
    x += dx;
    y += dy;

    // Check for collisions with screen edges and reverse direction if needed
    if (x <= 0 || x + squareSize >= 128) dx = -dx; // Reverse X direction
    if (y <= 0 || y + squareSize >= 64) dy = -dy;  // Reverse Y direction

    // Send the buffer to the display
    u8g2.sendBuffer();

    // Add a small delay for smooth animation
    delay(50);
}

} // namespace UC1701