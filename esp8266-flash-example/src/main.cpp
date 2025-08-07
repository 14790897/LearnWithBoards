#include <Arduino.h>

void setup() {
  Serial.begin(115200); // 初始化串口，波特率115200
  pinMode(LED_BUILTIN, OUTPUT); // 初始化板载LED为输出模式
}

void loop() {
  digitalWrite(LED_BUILTIN, LOW);  // 点亮LED（ESP8266低电平点亮）
  Serial.println("LED is ON");     // 输出调试信息到串口
  delay(1000);                     // 延时1秒
  digitalWrite(LED_BUILTIN, HIGH); // 熄灭LED
  Serial.println("LED is OFF");    // 输出调试信息到串口
  delay(1000);                     // 延时1秒
}