#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include "ST7789.h"

// ST7789V 引脚定义
#define TFT_CS    7   // 片选引脚
#define TFT_DC    6   // 数据/命令引脚
#define TFT_RST   10  // 复位引脚

// 创建 ST7789 对象
Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);

namespace ST7789 {
    void setup() {
        // 初始化串口调试
        Serial.begin(115200);
        Serial.println("Adafruit ST7789 初始化...");

        // 初始化显示屏
        tft.init(240, 240); // 初始化 ST7789，分辨率为 240x240
        tft.setRotation(1); // 设置屏幕方向
        tft.fillScreen(ST77XX_BLACK); // 清屏为黑色

        // 显示文字
        tft.setTextColor(ST77XX_WHITE);
        tft.setTextSize(2);
        tft.setCursor(10, 30);
        tft.print("Hello, ST7789!");
    }

    void loop() {
        int x = 0, y = 0; // 矩形的初始位置
        int dx = 5, dy = 5; // 矩形移动的步长
        int rectWidth = 50, rectHeight = 50; // 矩形的宽度和高度
        uint16_t colors[] = {ST77XX_RED, ST77XX_GREEN, ST77XX_BLUE, ST77XX_YELLOW, ST77XX_CYAN, ST77XX_MAGENTA};
        int colorIndex = 0;

        while (true) {
            // 清屏
            tft.fillScreen(ST77XX_BLACK);

            // 绘制矩形
            tft.fillRect(x, y, rectWidth, rectHeight, colors[colorIndex]);

            // 更新矩形位置
            x += dx;
            y += dy;

            // 碰到边界时反弹并切换颜色
            if (x <= 0 || x + rectWidth >= 240) {
                dx = -dx;
                colorIndex = (colorIndex + 1) % 6; // 切换颜色
            }
            if (y <= 0 || y + rectHeight >= 320) {
                dy = -dy;
                colorIndex = (colorIndex + 1) % 6; // 切换颜色
            }

            // 延时以控制动画速度
            delay(50);
        }
    }
}
