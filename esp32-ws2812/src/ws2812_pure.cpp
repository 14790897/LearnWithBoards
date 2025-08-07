#include <Arduino.h>
#include <FastLED.h>

// 编译时配置，可在platformio.ini中设置
#ifndef LED_PIN
#define LED_PIN 18       // WS2812 控制信号引脚
#endif

#ifndef NUM_LEDS
#define NUM_LEDS 60 // 灯带灯珠数量
#endif

#ifndef BRIGHTNESS
#define BRIGHTNESS 100   // 灯带亮度 (0-255)
#endif

#define LED_TYPE WS2812B // 灯带型号
#define COLOR_ORDER GRB  // WS2812 默认排列

CRGB leds[NUM_LEDS];

// 效果模式
enum EffectMode
{
  RAINBOW_FLOW,   // 彩虹流动
  COLOR_WIPE,     // 颜色扫过
  RAINBOW_SPIRAL, // 彩虹螺旋
  FIRE_WORKS,     // 烟花效果
  TWINKLE,        // 闪烁效果
  CHASE,          // 追逐效果
  FIRE,           // 火焰效果
  METEOR,         // 流星雨
  RAINBOW_CYCLE,  // 彩虹循环
  PLASMA,         // 等离子效果
  LIGHTNING,      // 闪电效果
  RAINBOW_STROBE, // 彩虹频闪
  KNIGHT_RIDER,   // 霹雳游侠
  COMET,          // 彗星效果
  SPARKLE_POP,    // 爆裂闪烁
  WAVE_COLLAPSE,  // 波浪坍塌
  DIGITAL_RAIN,   // 数字雨
  PULSE_WAVE      // 脉冲波
};

EffectMode currentEffect = RAINBOW_FLOW;
unsigned long lastEffectChange = 0;
const unsigned long EFFECT_DURATION = 6000; // 每种效果持续3秒

void setup()
{
  Serial.begin(115200);
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(BRIGHTNESS);

  Serial.println("WS2812 华丽灯带控制程序启动");
  Serial.print("LED数量: ");
  Serial.println(NUM_LEDS);
  Serial.print("控制引脚: ");
  Serial.println(LED_PIN);
  Serial.print("亮度设置: ");
  Serial.println(BRIGHTNESS);
  Serial.println("包含18种华丽效果，每6秒自动切换");
}

void rainbowFlow() {
  static uint8_t hue = 0;
  static uint8_t direction = 1;
  
  for (int i = 0; i < NUM_LEDS; i++) {
    // 更流畅的彩虹效果，增加方向变化
    uint8_t pixelHue = hue + (i * 256 / NUM_LEDS);
    leds[i] = CHSV(pixelHue, 255, 255);
  }
  
  hue += direction * 2;
  if (hue > 250 || hue < 5) direction *= -1; // 反向流动
}

void colorWipeEffect()
{
  static int position = 0;
  static uint8_t hue = 0;
  static bool forward = true;
  static unsigned long lastMove = 0;

  if (millis() - lastMove > 80)
  {
    if (forward)
    {
      leds[position] = CHSV(hue, 255, 255);
      position++;
      if (position >= NUM_LEDS)
      {
        forward = false;
        position = NUM_LEDS - 1;
        hue += 30; // 换颜色
      }
    }
    else
    {
      leds[position] = CRGB::Black;
      position--;
      if (position < 0)
      {
        forward = true;
        position = 0;
      }
    }
    lastMove = millis();
  }
}

void rainbowSpiralEffect()
{
  static float angle = 0;
  static uint8_t hueOffset = 0;

  angle += 0.1;
  hueOffset += 2;

  for (int i = 0; i < NUM_LEDS; i++) {
    float spiralPos = sin(angle + i * 0.3) * 0.5 + 0.5; // 0-1范围
    uint8_t hue = hueOffset + (i * 4) + (spiralPos * 60);
    uint8_t brightness = 150 + (spiralPos * 105); // 150-255范围
    leds[i] = CHSV(hue, 255, brightness);
  }
}

void fireWorksEffect()
{
  static unsigned long lastFirework = 0;
  static bool exploding = false;
  static int centerPos = 0;
  static uint8_t explosionRadius = 0;
  static uint8_t fireworkHue = 0;
  static unsigned long explosionStart = 0;

  // 所有LED逐渐变暗
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i].fadeToBlackBy(15);
  }

  if (!exploding && millis() - lastFirework > random(1000, 3000))
  {
    // 开始新的烟花
    exploding = true;
    centerPos = random(5, NUM_LEDS - 5);
    explosionRadius = 0;
    fireworkHue = random8();
    explosionStart = millis();
    lastFirework = millis();
  }

  if (exploding)
  {
    if (millis() - explosionStart < 500)
    { // 爆炸持续500ms
      explosionRadius = map(millis() - explosionStart, 0, 500, 0, 8);

      // 绘制爆炸效果
      for (int i = 0; i <= explosionRadius; i++)
      {
        uint8_t brightness = 255 - (i * 25);
        if (centerPos - i >= 0)
        {
          leds[centerPos - i] = CHSV(fireworkHue, 255, brightness);
        }
        if (centerPos + i < NUM_LEDS)
        {
          leds[centerPos + i] = CHSV(fireworkHue, 255, brightness);
        }
      }
    }
    else
    {
      exploding = false;
    }
  }
}

void twinkleEffect() {
  // 随机闪烁
  if (random8() < 50) { // 约20%的概率
    int led = random8(NUM_LEDS);
    leds[led] = CHSV(random8(), 255, 255);
  }
  
  // 逐渐变暗
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i].fadeToBlackBy(20);
  }
}

void chaseEffect() {
  static int position = 0;
  static unsigned long lastMove = 0;
  static uint8_t hue = 0;
  
  if (millis() - lastMove > 150) { // 每150ms移动一次
    fill_solid(leds, NUM_LEDS, CRGB::Black);
    
    // 多彩追逐效果
    for (int i = 0; i < 5; i++) {
      int pos = (position + i) % NUM_LEDS;
      uint8_t brightness = 255 - (i * 50);
      leds[pos] = CHSV(hue + (i * 30), 255, brightness);
    }
    
    position = (position + 1) % NUM_LEDS;
    hue += 3;
    lastMove = millis();
  }
}

// 新增华丽效果函数
void fireEffect() {
  static byte heat[NUM_LEDS];
  
  // 降温
  for (int i = 0; i < NUM_LEDS; i++) {
    heat[i] = qsub8(heat[i], random8(0, ((55 * 10) / NUM_LEDS) + 2));
  }
  
  // 热量向上传播
  for (int k = NUM_LEDS - 1; k >= 2; k--) {
    heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2]) / 3;
  }
  
  // 底部随机点燃
  if (random8() < 120) {
    int y = random8(7);
    heat[y] = qadd8(heat[y], random8(160, 255));
  }
  
  // 映射到颜色
  for (int j = 0; j < NUM_LEDS; j++) {
    CRGB color = HeatColor(heat[j]);
    leds[j] = color;
  }
}

void meteorEffect() {
  static int meteorPos = -10;
  static uint8_t meteorHue = 0;
  
  // 淡化所有像素
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i].fadeToBlackBy(20);
  }
  
  // 绘制流星
  for (int i = 0; i < 8; i++) {
    int pos = meteorPos - i;
    if (pos >= 0 && pos < NUM_LEDS) {
      uint8_t brightness = 255 - (i * 30);
      leds[pos] = CHSV(meteorHue, 255, brightness);
    }
  }
  
  meteorPos++;
  if (meteorPos > NUM_LEDS + 10) {
    meteorPos = -10;
    meteorHue += 60; // 换颜色
  }
}

void rainbowCycleEffect() {
  static uint16_t j = 0;
  
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = CHSV(((i * 256 / NUM_LEDS) + j) & 255, 255, 255);
  }
  j += 2;
}

void plasmaEffect() {
  static float phase1 = 0, phase2 = 0, phase3 = 0;
  
  phase1 += 0.1;
  phase2 += 0.05;
  phase3 += 0.07;
  
  for (int i = 0; i < NUM_LEDS; i++) {
    float v1 = sin(phase1 + i * 0.3);
    float v2 = sin(phase2 + i * 0.2);
    float v3 = sin(phase3 + i * 0.1);
    
    uint8_t brightness = (v1 + v2 + v3 + 3) * 42;
    uint8_t hue = (v1 + v2) * 127 + 128;
    
    leds[i] = CHSV(hue, 255, brightness);
  }
}

void lightningEffect() {
  static unsigned long lastFlash = 0;
  static bool flashing = false;
  static int flashCount = 0;
  static uint8_t lightningHue = 0;

  if (!flashing && millis() - lastFlash > random(2000, 5000)) {
    flashing = true;
    flashCount = random(1, 4);
    lightningHue = random8(180, 255); // 蓝紫色调的闪电
    lastFlash = millis();
  }
  
  if (flashing) {
    if (flashCount > 0 && millis() - lastFlash > random(50, 200)) {
      // 彩色闪电效果
      fill_solid(leds, NUM_LEDS, CHSV(lightningHue, 200, 255));
      FastLED.show();
      delay(random(10, 50));
      fill_solid(leds, NUM_LEDS, CRGB::Black);
      flashCount--;
      lastFlash = millis();
      lightningHue += random8(10, 30); // 每次闪电稍微变色
    }
    
    if (flashCount <= 0) {
      flashing = false;
    }
  } else {
    // 平时保持暗蓝色
    for (int i = 0; i < NUM_LEDS; i++) {
      leds[i] = CRGB(0, 0, 20);
    }
  }
}

void rainbowStrobeEffect()
{
  static unsigned long lastStrobe = 0;
  static uint8_t strobeHue = 0;
  static bool strobeOn = false;
  static int strobeCount = 0;
  static int maxStrobes = 0;

  if (strobeCount <= 0)
  {
    // 开始新一轮频闪
    maxStrobes = random(3, 8); // 3-7次频闪
    strobeCount = maxStrobes;
    strobeHue = random8(); // 随机颜色
    lastStrobe = millis();
  }

  if (millis() - lastStrobe > 100)
  { // 每100ms切换一次
    if (strobeOn)
    {
      // 关闭频闪
      fill_solid(leds, NUM_LEDS, CRGB::Black);
      strobeOn = false;
      strobeCount--;

      if (strobeCount <= 0)
      {
        // 这轮频闪结束，等待下一轮
        lastStrobe = millis() + random(500, 2000); // 等待0.5-2秒
      }
    }
    else
    {
      // 开启频闪
      fill_solid(leds, NUM_LEDS, CHSV(strobeHue, 255, 255));
      strobeOn = true;
    }
    lastStrobe = millis();
  }
}

void knightRiderEffect() {
  static int position = 0;
  static int direction = 1;
  static unsigned long lastMove = 0;
  
  if (millis() - lastMove > 100) {
    fill_solid(leds, NUM_LEDS, CRGB::Black);
    
    // 红色扫描
    for (int i = 0; i < 4; i++) {
      int pos = position + (i * direction);
      if (pos >= 0 && pos < NUM_LEDS) {
        uint8_t brightness = 255 - (i * 60);
        leds[pos] = CRGB(brightness, 0, 0);
      }
    }
    
    position += direction;
    if (position >= NUM_LEDS - 3 || position <= 0) {
      direction *= -1;
    }
    
    lastMove = millis();
  }
}

void cometEffect() {
  static int cometPos = 0;
  static uint8_t cometHue = 0;
  
  // 淡化尾迹
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i].fadeToBlackBy(15);
  }
  
  // 彗星头部
  leds[cometPos] = CHSV(cometHue, 255, 255);
  
  // 彗星尾巴
  for (int i = 1; i < 6 && cometPos - i >= 0; i++) {
    uint8_t brightness = 255 - (i * 40);
    leds[cometPos - i] += CHSV(cometHue, 255, brightness);
  }
  
  cometPos++;
  if (cometPos >= NUM_LEDS) {
    cometPos = 0;
    cometHue += 30;
  }
}

void sparklePopEffect() {
  static unsigned long lastPop = 0;
  
  // 所有LED逐渐变暗
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i].fadeToBlackBy(8);
  }
  
  // 随机爆裂
  if (millis() - lastPop > random(100, 500)) {
    int center = random8(NUM_LEDS);
    uint8_t hue = random8();
    
    // 中心最亮
    leds[center] = CHSV(hue, 255, 255);
    
    // 周围扩散
    for (int i = 1; i < 4; i++) {
      if (center - i >= 0) leds[center - i] = CHSV(hue, 255, 255 - i * 50);
      if (center + i < NUM_LEDS) leds[center + i] = CHSV(hue, 255, 255 - i * 50);
    }

    lastPop = millis();
  }
}

void waveCollapseEffect()
{
  static float wavePos = 0;
  static bool expanding = true;
  static uint8_t waveHue = 0;

  // 清空所有LED
  fill_solid(leds, NUM_LEDS, CRGB::Black);

  if (expanding)
  {
    wavePos += 0.5;
    if (wavePos > NUM_LEDS / 2)
    {
      expanding = false;
      waveHue += 60;
    }
  }
  else
  {
    wavePos -= 0.3;
    if (wavePos < 0)
    {
      expanding = true;
      wavePos = 0;
    }
  }

  // 从中心向外扩散或收缩
  int center = NUM_LEDS / 2;
  for (int i = 0; i < wavePos; i++)
  {
    uint8_t brightness = 255 - (i * 8);
    if (center - i >= 0)
    {
      leds[center - i] = CHSV(waveHue, 255, brightness);
    }
    if (center + i < NUM_LEDS)
    {
      leds[center + i] = CHSV(waveHue, 255, brightness);
    }
  }
}

void digitalRainEffect()
{
  static byte drops[NUM_LEDS];
  static uint8_t speed[NUM_LEDS];
  static uint8_t brightness[NUM_LEDS];
  static uint8_t hue[NUM_LEDS];

  // 随机生成新数字雨滴
  if (random8() < 40)
  {
    int pos = random8(NUM_LEDS);
    drops[pos] = random(1, 4); // 不同类型的雨滴
    speed[pos] = random(2, 6);
    brightness[pos] = 255;
    hue[pos] = random8(180, 220); // 蓝绿色调
  }

  // 更新雨滴
  for (int i = 0; i < NUM_LEDS; i++)
  {
    if (drops[i] > 0)
    {
      // 根据雨滴类型选择颜色
      uint8_t dropHue = hue[i] + (drops[i] * 20);
      leds[i] = CHSV(dropHue, 255, brightness[i]);

      brightness[i] = brightness[i] * 0.92; // 逐渐变暗

      if (brightness[i] < 15)
      {
        drops[i] = 0;
      }
    }
    else
    {
      leds[i] = CRGB::Black;
    }
  }
}

void pulseWaveEffect()
{
  static float pulse1 = 0, pulse2 = 0, pulse3 = 0;
  static uint8_t baseHue = 0;

  pulse1 += 0.15;
  pulse2 += 0.08;
  pulse3 += 0.12;
  baseHue += 1;

  for (int i = 0; i < NUM_LEDS; i++)
  {
    float pos = (float)i / NUM_LEDS;

    // 三个不同频率的脉冲波
    float wave1 = sin(pulse1 + pos * 8) * 0.5 + 0.5;
    float wave2 = sin(pulse2 + pos * 12) * 0.3 + 0.3;
    float wave3 = sin(pulse3 + pos * 6) * 0.2 + 0.2;

    float combinedWave = (wave1 + wave2 + wave3) / 3.0;

    uint8_t brightness = combinedWave * 255;
    uint8_t hue = baseHue + (pos * 120) + (combinedWave * 60);

    leds[i] = CHSV(hue, 255, brightness);
  }
}

void switchEffect() {
  if (millis() - lastEffectChange > EFFECT_DURATION) {
    currentEffect = (EffectMode)((currentEffect + 1) % 18); // 更新为18种效果
    lastEffectChange = millis();
    
    // 清空灯带
    fill_solid(leds, NUM_LEDS, CRGB::Black);
    FastLED.show();
    delay(100);
    
    // 输出当前效果
    Serial.print("切换到效果: ");
    switch (currentEffect) {
      case RAINBOW_FLOW: Serial.println("彩虹流动"); break;
      case COLOR_WIPE:
        Serial.println("颜色扫过");
        break;
      case RAINBOW_SPIRAL:
        Serial.println("彩虹螺旋");
        break;
      case FIRE_WORKS:
        Serial.println("烟花效果");
        break;
      case TWINKLE: Serial.println("闪烁效果"); break;
      case CHASE: Serial.println("追逐效果"); break;
      case FIRE: Serial.println("火焰效果"); break;
      case METEOR: Serial.println("流星雨"); break;
      case RAINBOW_CYCLE:
        Serial.println("彩虹循环");
        break;
      case PLASMA:
        Serial.println("等离子效果");
        break;
      case LIGHTNING: Serial.println("闪电效果"); break;
      case RAINBOW_STROBE:
        Serial.println("彩虹频闪");
        break;
      case KNIGHT_RIDER: Serial.println("霹雳游侠"); break;
      case COMET: Serial.println("彗星效果"); break;
      case SPARKLE_POP: Serial.println("爆裂闪烁"); break;
      case WAVE_COLLAPSE:
        Serial.println("波浪坍塌");
        break;
      case DIGITAL_RAIN:
        Serial.println("数字雨");
        break;
      case PULSE_WAVE:
        Serial.println("脉冲波");
        break;
      }
  }
}

void loop()
{
  switchEffect(); // 检查是否需要切换效果
    // 执行当前效果
  switch (currentEffect) {
    case RAINBOW_FLOW:
      rainbowFlow();
      break;
    case COLOR_WIPE:
      colorWipeEffect();
      break;
    case RAINBOW_SPIRAL:
      rainbowSpiralEffect();
      break;
    case FIRE_WORKS:
      fireWorksEffect();
      break;
    case TWINKLE:
      twinkleEffect();
      break;
    case CHASE:
      chaseEffect();
      break;
    case FIRE:
      fireEffect();
      break;
    case METEOR:
      meteorEffect();
      break;
    case RAINBOW_CYCLE:
      rainbowCycleEffect();
      break;
    case PLASMA:
      plasmaEffect();
      break;
    case LIGHTNING:
      lightningEffect();
      return; // 闪电效果自己控制FastLED.show()
    case RAINBOW_STROBE:
      rainbowStrobeEffect();
      break;
    case KNIGHT_RIDER:
      knightRiderEffect();
      break;
    case COMET:
      cometEffect();
      break;
    case SPARKLE_POP:
      sparklePopEffect();
      break;
    case WAVE_COLLAPSE:
      waveCollapseEffect();
      break;
    case DIGITAL_RAIN:
      digitalRainEffect();
      break;
    case PULSE_WAVE:
      pulseWaveEffect();
      break;
    }

  FastLED.show();
  delay(50); // 控制刷新率
}
