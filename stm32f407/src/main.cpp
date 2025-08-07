#include <Arduino.h>

// 简化常量定义
#define LED_PIN PC13
#define ANALOG_PIN PA0
#define BLINK_INTERVAL 1000

// 全局变量
unsigned long previousMillis = 0;
bool ledState = LOW;

void setup() {
  // 初始化 LED 和串口
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);
  
  Serial.begin(115200);
  delay(2000);  // 等待串口稳定
  
  // 输出启动信息
  Serial.println("STM32F407 Ready");
  digitalWrite(LED_PIN, LOW);
  Serial.println("Setup OK");
}


void loop() {
  unsigned long now = millis();

  if (now - previousMillis >= BLINK_INTERVAL) {
    previousMillis = now;

    ledState = !ledState;
    digitalWrite(LED_PIN, ledState);

    int analogValue = analogRead(ANALOG_PIN);
    
    // 简化的输出
    Serial.print("LED: ");
    Serial.print(ledState ? "ON " : "OFF");
    Serial.print(" | ADC: ");
    Serial.println(analogValue);
  }
  
  // 检查串口输入
  if (Serial.available()) {
    String input = Serial.readString();
    input.trim();
    Serial.print("RX: ");
    Serial.println(input);
  }
}
