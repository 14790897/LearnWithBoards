#include <Arduino.h>

#define LED_PIN 12 // 修改为 GPIO 8

void setup()
{
  pinMode(LED_PIN, OUTPUT); // 将 LED 引脚设置为输出模式
}

void loop()
{
  digitalWrite(LED_PIN, HIGH); // 点亮 LED
  delay(500);                  // 延时 500 毫秒
  digitalWrite(LED_PIN, LOW);  // 熄灭 LED
  delay(500);                  // 延时 500 毫秒
}
