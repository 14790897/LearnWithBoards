#include <Arduino.h>

const int sensorPin = 18; // 你接在 ESP32 的 GPIO15 上

void setup() {
  Serial.begin(115200);
  pinMode(sensorPin, INPUT);  // 设置为输入模式
}

void loop() {
  int state = digitalRead(sensorPin);

  if (state == HIGH) {
    Serial.println("检测到高电平");
  } else {
    // Serial.println("检测到低电平");
  }

  delay(200); // 每200ms读取一次
}
