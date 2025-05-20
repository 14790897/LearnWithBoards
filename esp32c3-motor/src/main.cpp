#include <Arduino.h>

// 设置用于控制马达的引脚
const int motorPin = 6;

void setup()
{
  // 设置 motorPin 为输出模式
  pinMode(motorPin, OUTPUT);

  // 启动 PWM 频道（LEDC 通道配置）
  ledcSetup(0, 5000, 8);      // 通道 0，频率 5kHz，分辨率 8 位
  ledcAttachPin(motorPin, 0); // 将 motorPin 绑定到通道 0
}

void loop()
{
  // 渐增速度
  for (int speed = 0; speed <= 255; speed += 5)
  {
    ledcWrite(0, speed);
    delay(20);
  }

  delay(2000);
}
