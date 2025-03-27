#include <Arduino.h>

// 定义两个 LED 的引脚
const int led1 = PC13; // 第一个 LED 连接到 PC13
const int led2 = PA5;  // 第二个 LED 连接到 PA5（假设）

void setup()
{
  // 设置两个引脚为输出模式
  pinMode(led1, OUTPUT);
  pinMode(led2, OUTPUT);
}

void loop()
{
  // 点亮两个 LED（低电平点亮）
  digitalWrite(led1, LOW);
  digitalWrite(led2, LOW);
  delay(1000); // 延迟 1 秒

  // 熄灭两个 LED（高电平熄灭）
  digitalWrite(led1, HIGH);
  digitalWrite(led2, HIGH);
  delay(1000); // 延迟 1 秒
}
