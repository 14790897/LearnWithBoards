#include <Arduino.h>

// 定义 LED 引脚（根据你的硬件调整）
#define LED_PIN 2 // 常见板载 LED 引脚为 GPIO 2

void setup()
{
  // 初始化串口（用于调试，可选）
  Serial.begin(115200);
  Serial.println("ESP32-S LED Blink Starting...");

  // 设置 LED 引脚为输出模式
  pinMode(LED_PIN, OUTPUT);
}

void loop()
{
  // 点亮 LED
  digitalWrite(LED_PIN, HIGH);
  Serial.println("LED ON");
  delay(1000); // 等待 1 秒

  // 熄灭 LED
  digitalWrite(LED_PIN, LOW);
  Serial.println("LED OFF");
  delay(1000); // 等待 1 秒
}