#include <Arduino.h>

// === 常量 ===
const int LED_PIN      = PC13;  // Black F407 板载 LED
const int ANALOG_PIN   = PA0;   // ADC0
const unsigned BLINK_INTERVAL = 1000;  // ms

// === 变量 ===
unsigned long previousMillis = 0;
bool ledState = LOW;

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 2000);   // 最多等 2 秒让 USB CDC 枚举

  Serial.println("STM32F407 Demo Program");

  pinMode(LED_PIN, OUTPUT);
  // analogRead 不必专门 pinMode
  digitalWrite(LED_PIN, ledState);
  analogReadResolution(12);             // 可选：明确 12bit
}

void loop() {
  unsigned long now = millis();

  if (now - previousMillis >= BLINK_INTERVAL) {
    previousMillis = now;

    ledState = !ledState;
    digitalWrite(LED_PIN, ledState);

    int analogValue = analogRead(ANALOG_PIN);

    Serial.print("LED: ");
    Serial.print(ledState ? "ON " : "OFF");
    Serial.print(" | ADC: ");
    Serial.println(analogValue);
  }

  // 其余后台任务……
}
