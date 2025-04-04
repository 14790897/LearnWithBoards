#include <Arduino.h>
#include "midi2wave.h"
#include "midi_data.h"

#define POW2_32 4294967296ULL
#define REFCLK 40000.0

// MIDI 音高到频率表（Hz）
const float PIANO[] = {
    0, 0, 0, 0, 0, 0, 0, 0, 8.18, 8.66, 9.18, 9.72,                 // 0-11
    10.30, 10.91, 11.56, 12.25, 12.98, 13.75, 14.57, 15.43,         // 12-23
    16.35, 17.32, 18.35, 19.45, 20.60, 21.83, 23.12, 24.50,         // 24-35
    25.96, 27.50, 29.14, 30.87, 32.70, 34.65, 36.71, 38.89,         // 36-47
    41.20, 43.65, 46.25, 49.00, 51.91, 55.00, 58.27, 61.74,         // 48-59
    65.41, 69.30, 73.42, 77.78, 82.41, 87.31, 92.50, 98.00,         // 60-71
    103.83, 110.00, 116.54, 123.47, 130.81, 138.59, 146.83, 155.56, // 72-83
    164.81, 174.61, 185.00, 196.00, 207.65, 220.00, 233.08, 246.94, // 84-95
    261.63, 277.18, 293.66, 311.13, 329.63, 349.23, 369.99, 392.00, // 96-107
    415.30, 440.00, 466.16, 493.88, 523.25, 554.37, 587.33, 622.25, // 108-119
    659.25, 698.46, 739.99, 783.99, 830.61, 880.00, 932.33, 987.77, // 120-127
};

volatile int timer_tick = 0;
volatile unsigned char timer_micro = 0;
volatile unsigned short timer_milli = 0;
volatile unsigned long phaccu_1, phaccu_2, phaccu_3, phaccu_4;
volatile unsigned long tword_m_1, tword_m_2, tword_m_3, tword_m_4;
int phaccu_all;

hw_timer_t *timer = NULL;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

#define BUZZER_PIN 13
#define PWM_CHANNEL 0
#define PWM_FREQ 20000  // 降低PWM频率
#define PWM_RESOLUTION 8

void IRAM_ATTR onTimer()
{
  portENTER_CRITICAL_ISR(&timerMux);
  
  // 快速计算相位累加
  phaccu_1 += tword_m_1;
  phaccu_2 += tword_m_2;
  phaccu_3 += tword_m_3;
  phaccu_4 += tword_m_4;

  // 计算合成波形
  int wave1 = pgm_read_word(&sine[phaccu_1 >> 24]);
  int wave2 = pgm_read_word(&sine[phaccu_2 >> 24]);
  int wave3 = pgm_read_word(&sine[phaccu_3 >> 24]);
  int wave4 = pgm_read_word(&sine[phaccu_4 >> 24]);

  // 混合波形并调整音量
  phaccu_all = (wave1 + wave2 + wave3 + wave4) >> 2;
  
  // 确保PWM值在0-255范围内
  phaccu_all = constrain(phaccu_all, 0, 255);
  
  // 输出PWM
  ledcWrite(PWM_CHANNEL, phaccu_all);

  // 更新时间计数器
  if (++timer_micro >= 40) {
    timer_micro = 0;
    timer_milli++;
  }
  
  portEXIT_CRITICAL_ISR(&timerMux);
}

void setup()
{
  Serial.begin(115200);
  Serial.println("Hello ESP32 MIDI Player");

  // 配置PWM
  ledcSetup(PWM_CHANNEL, PWM_FREQ, PWM_RESOLUTION);
  ledcAttachPin(BUZZER_PIN, PWM_CHANNEL);
  ledcWrite(PWM_CHANNEL, 0);  // 初始输出为0

  // 测试PWM输出
  Serial.println("Testing PWM output...");
  for(int i = 0; i < 3; i++) {
    ledcWrite(PWM_CHANNEL, 128);  // 50%占空比
    delay(500);
    ledcWrite(PWM_CHANNEL, 0);
    delay(500);
  }

  setupMidi();

  timer = timerBegin(0, 80, true);
  timerAttachInterrupt(timer, &onTimer, true);
  timerAlarmWrite(timer, 25, true);
  timerAlarmEnable(timer);

  tword_m_1 = 0;
  tword_m_2 = 0;
  tword_m_3 = 0;
  tword_m_4 = 0;
  
  Serial.println("Setup complete");
}

void loop()
{
  if (timer_milli > event_length)
  {
    timerAlarmDisable(timer);
    loadNextEvent();
    
    // 批量计算频率
    float freq1 = PIANO[active_keys[0]];
    float freq2 = PIANO[active_keys[1]];
    float freq3 = PIANO[active_keys[2]];
    float freq4 = PIANO[active_keys[3]];

    // 计算相位增量
    tword_m_1 = freq1 > 0 ? (unsigned long)(POW2_32 * freq1 / REFCLK) : 0;
    tword_m_2 = freq2 > 0 ? (unsigned long)(POW2_32 * freq2 / REFCLK) : 0;
    tword_m_3 = freq3 > 0 ? (unsigned long)(POW2_32 * freq3 / REFCLK) : 0;
    tword_m_4 = freq4 > 0 ? (unsigned long)(POW2_32 * freq4 / REFCLK) : 0;

    // 重置相位累加器
    phaccu_1 = 0;
    phaccu_2 = 0;
    phaccu_3 = 0;
    phaccu_4 = 0;

    // 打印调试信息
    Serial.printf("Frequencies: %.2f, %.2f, %.2f, %.2f\n", freq1, freq2, freq3, freq4);
    Serial.printf("Phase increments: %lu, %lu, %lu, %lu\n", tword_m_1, tword_m_2, tword_m_3, tword_m_4);

    timer_milli = 0;
    timerAlarmEnable(timer);
  }
  
  delayMicroseconds(100);
  yield();
}