#include <Arduino.h>

#include <FastLED.h>

#define LED_PIN 18 // 控制信号连接的 GPIO，比如 D2
#define TRIG_PIN 3 // 超声波传感器触发引脚
#define ECHO_PIN 10
#define NUM_LEDS 8       // 灯珠数量，可根据你的灯带实际调整
#define BRIGHTNESS 100   // 亮度（0~255）
#define LED_TYPE WS2812B // 型号
#define COLOR_ORDER GRB  // WS2812 默认是 GRB 排列

static int smoothBrightness = 50; // 初始亮度

CRGB leds[NUM_LEDS];

void setup()
{
  Serial.begin(115200);
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(BRIGHTNESS);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
}

void loop()
{
  unsigned long startTime = millis(); // 记录循环开始时间

  // 超声波测距
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, 30000); // 30ms超时
  if (duration == 0)
  {
    Serial.println("未检测到回波，跳过本次测量");
    delay(200);
    return;
  }
  float distance = duration * 0.034 / 2; // 单位cm

  int targetBrightness = map((int)distance, 1, 50, 255, 1);
  targetBrightness = constrain(targetBrightness, 1, 255);

  // 低通滤波亮度平滑处理，0.2可以调整平滑程度，越小越平滑
  smoothBrightness = smoothBrightness + 0.2 * (targetBrightness - smoothBrightness);

  FastLED.setBrightness(smoothBrightness);

  // 彩虹流动效果
  for (int i = 0; i < NUM_LEDS; i++)
  {
    leds[i] = CHSV((millis() / 10 + i * 10) % 255, 255, 255);
  }
  FastLED.show();

  Serial.print("距离: ");
  Serial.print(distance);
  Serial.print(" cm, 亮度: ");
  Serial.println(smoothBrightness);

  delay(100);
  unsigned long endTime = millis(); // 记录循环结束时间
  Serial.print("本次循环耗时: ");
  Serial.print(endTime - startTime);
  Serial.println(" ms");
}
