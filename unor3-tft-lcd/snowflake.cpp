#include <Arduino.h>
#include <SPI.h>
#include <Ucglib.h>

// 初始化 ST7735 显示屏 (128x160, 软件 SPI)
Ucglib_ST7735_18x128x160_HWSPI  ucg(9, 10, 8); // sclk, data, cd, cs, reset

struct Snowflake
{
  int x;       // 雪花的 x 坐标
  int y;       // 雪花的 y 坐标
  int size;    // 雪花的大小
  bool active; // 是否激活
};

#define MAX_SNOWFLAKES 100
Snowflake snowflakes[MAX_SNOWFLAKES];

void setup()
{
  Serial.begin(115200); // 添加串口调试
  delay(1000);

  ucg.begin(UCG_FONT_MODE_TRANSPARENT); // 初始化显示屏

  ucg.setRotate90(); // 旋转 90 度，变为 160x128
  ucg.clearScreen(); // 清屏

  // 初始化雪花
  for (int i = 0; i < MAX_SNOWFLAKES; i++)
  {
    snowflakes[i].x = random(0, 160);  // 随机 x 位置（旋转后宽度 160）
    snowflakes[i].y = -10;              // 从屏幕上方开始
    snowflakes[i].size = random(2, 5); // 随机大小（2 到 4 像素）
    snowflakes[i].active = false;      // 初始未激活
  }

  // 显示初始文本
  ucg.setFont(ucg_font_ncenR14_tr);
  ucg.setPrintPos(30, 65);
  ucg.setColor(255, 255, 255); // 白色（RGB 全 255）
  ucg.print("Hello World!");
  delay(500);
}

void loop()
{
  ucg.setColor(0, 0, 0);       // 黑色背景
  ucg.drawBox(0, 0, 160, 128); // 清屏（旋转后 160x128）

  // 更新并绘制雪花
  for (int i = 0; i < MAX_SNOWFLAKES; i++)
  {
    // 如果雪花未激活，随机激活
    if (!snowflakes[i].active && random(100) < 30)
    {
      snowflakes[i].x = random(0, 160);
      snowflakes[i].y = 20;
      snowflakes[i].active = true;
      // Serial.print("Snowflake %d activated at x=%d, y=%d\n", i, snowflakes[i].x, snowflakes[i].y);
    }

    // 如果雪花激活，更新位置
    if (snowflakes[i].active)
    {
      snowflakes[i].y += 1; // 向下移动

      // 绘制雪花
      ucg.setColor(255, 255, 255);                                        // 白色
      ucg.drawCircle(snowflakes[i].x, snowflakes[i].y, snowflakes[i].size,1); // Ucglib 无需 U8G2_DRAW_ALL

      // 如果超出屏幕底部，重置
      if (snowflakes[i].y >= 128)
      {
        snowflakes[i].active = false;
        // Serial.print("Snowflake %d reset\n", i);
      }
    }
  }

  delay(5); // 动画速度
}