#include <Arduino.h>
// 定义继电器控制引脚
const int relayPin = 2; // 继电器模块的控制引脚

void setup()
{
  Serial.begin(115200);
  pinMode(relayPin, OUTPUT);   // 设置继电器引脚为输出模式
  digitalWrite(relayPin, LOW); // 初始化继电器为关闭状态
}

void loop()
{
  digitalWrite(relayPin, HIGH); // 打开继电器（设备工作）
  Serial.println("Relay ON"); // 打印状态
  delay(500);                  // 保持5秒
  digitalWrite(relayPin, LOW); // 关闭继电器（设备停止）
  Serial.println("Relay OFF"); // 打印状态
  delay(500);                 // 保持5秒
}
