#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include "ILI9341.h"
#include <SPI.h>

namespace ILI9341
{
    // 定义引脚
    const uint8_t TFT_CS = 7;   // 片选
    const uint8_t TFT_DC = 6;   // 数据/命令
    const uint8_t TFT_RST = 10; // 复位

    // 使用硬件SPI初始化ILI9341
    Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);

    void setup()
    {
        Serial.begin(115200);
        while (!Serial)
        {
            delay(10); // 等待串口
        }

        Serial.println("Starting ILI9341 setup...");

        // 初始化显示
        Serial.println("Initializing ILI9341...");
        tft.begin();
        tft.setRotation(1); // 横屏显示
        tft.fillScreen(ILI9341_BLACK);

        // 显示初始化信息
        tft.setTextColor(ILI9341_WHITE);
        tft.setTextSize(2);
        tft.setCursor(10, 10);
        tft.print("ILI9341 Test");

        tft.setCursor(10, 30);
        tft.print("ESP32-C3");

        tft.setCursor(10, 50);
        tft.setTextColor(ILI9341_GREEN);
        tft.print("320x240 TFT");

        Serial.println("ILI9341 initialized");
        Serial.println("Setup complete.");

        delay(2000); // 显示初始化信息2秒
    }

    void ILI9341_loop()
    {
        static int x = 0;
        static int y = 100;
        static int oldX = 0;
        static int oldY = 100;
        static int dx = 3;
        static int dy = 2;
        static int rectWidth = 40;
        static int rectHeight = 25;
        static uint16_t colors[] = {
            ILI9341_RED, ILI9341_GREEN, ILI9341_BLUE,
            ILI9341_YELLOW, ILI9341_CYAN, ILI9341_MAGENTA,
            ILI9341_WHITE, ILI9341_ORANGE};
        static int colorIndex = 0;
        static unsigned long lastUpdate = 0;
        static bool initialized = false;

        // 只初始化一次背景
        if (!initialized)
        {
            tft.fillScreen(ILI9341_BLACK);

            // 绘制标题（只画一次）
            tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
            tft.setTextSize(2);
            tft.setCursor(10, 10);
            tft.print("ILI9341 Smooth");

            initialized = true;
        }

        unsigned long now = millis();
        if (now - lastUpdate >= 30) // 33FPS动画，更流畅
        {
            // 只清除旧矩形位置（用黑色覆盖）
            tft.fillRect(oldX - 1, oldY - 1, rectWidth + 2, rectHeight + 2, ILI9341_BLACK);

            // 更新位置
            x += dx;
            y += dy;

            // 边界检测和颜色变化
            if (x <= 0 || x + rectWidth >= 320)
            {
                dx = -dx;
                colorIndex = (colorIndex + 1) % 8;
            }
            if (y <= 40 || y + rectHeight >= 200) // 避免覆盖文字区域
            {
                dy = -dy;
                colorIndex = (colorIndex + 1) % 8;
            }

            // 画新位置的彩色矩形
            tft.fillRect(x, y, rectWidth, rectHeight, colors[colorIndex]);

            // 画边框
            tft.drawRect(x - 1, y - 1, rectWidth + 2, rectHeight + 2, ILI9341_WHITE);

            // 更新状态信息（覆盖式更新，减少闪烁）
            tft.setTextSize(1);
            tft.setTextColor(ILI9341_GREEN, ILI9341_BLACK);
            tft.setCursor(10, 220);
            tft.printf("Color:%d Pos:(%3d,%3d) ", colorIndex, x, y);

            // 保存当前位置为下次的旧位置
            oldX = x;
            oldY = y;

            lastUpdate = now;
        }

        // 添加一个简单的进度条动画（右下角）
        static unsigned long lastBarUpdate = 0;
        static int barProgress = 0;
        static bool barDirection = true;

        if (now - lastBarUpdate >= 20)
        {
            int barX = 200;
            int barY = 220;
            int barWidth = 100;
            int barHeight = 8;

            // 清除旧的进度条
            tft.drawRect(barX - 1, barY - 1, barWidth + 2, barHeight + 2, ILI9341_WHITE);
            tft.fillRect(barX, barY, barWidth, barHeight, ILI9341_BLACK);

            // 画新的进度条
            int fillWidth = (barProgress * barWidth) / 100;
            tft.fillRect(barX, barY, fillWidth, barHeight, ILI9341_CYAN);

            // 更新进度
            if (barDirection)
            {
                barProgress += 2;
                if (barProgress >= 100)
                    barDirection = false;
            }
            else
            {
                barProgress -= 2;
                if (barProgress <= 0)
                    barDirection = true;
            }

            lastBarUpdate = now;
        }
    }

} // namespace ILI9341
