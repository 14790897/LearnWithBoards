#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

// 创建 PCA9685 对象
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();

// 舵机的 PWM 范围
#define SERVOMIN 102 // 对应 0°（500μs）
#define SERVOMAX 512 // 对应 180°（2500μs）
#define SDA_PIN 4
#define SCL_PIN 5
// 初始化舵机通道
#define SERVO_CHANNEL_1 0 // 舵机 1 连接到 PCA9685 的通道 0
#define SERVO_CHANNEL_2 1 // 舵机 2 连接到 PCA9685 的通道 1
#define SERVO_CHANNEL_3 2 // 舵机 3 连接到 PCA9685 的通道 2
#define SERVO_CHANNEL_4 3 // 舵机 4 连接到 PCA9685 的通道 3
#define SERVO_CHANNEL_5 4 // 舵机 5 连接到 PCA9685 的通道 4
#define SERVO_CHANNEL_6 5 // 舵机 6 连接到 PCA9685 的通道 5
#define SERVO_CHANNEL_7 6 // 舵机 7 连接到 PCA9685 的通道 6
#define SERVO_CHANNEL_8 7 // 舵机 8 连接到 PCA9685 的通道 7

// 设置舵机角度函数
void setServoAngle(uint8_t channel, uint16_t angle)
{
  // 将角度转换为对应的 PWM 值
  uint16_t pulse = map(angle, 0, 180, SERVOMIN, SERVOMAX);
  pwm.setPWM(channel, 0, pulse);
}

void setup()
{
  Serial.begin(115200);
  Wire.begin(SDA_PIN, SCL_PIN);

  // 初始化 PCA9685
  pwm.begin();
  pwm.setPWMFreq(50); // 设置 PWM 频率为 50Hz，适用于舵机

  Serial.println("PCA9685 初始化完成，开始控制舵机...");
}

void loop()
{
  // 同时控制 8 个舵机
  setServoAngle(SERVO_CHANNEL_1, 0);   // 舵机 1 转到 0°
  setServoAngle(SERVO_CHANNEL_2, 45);  // 舵机 2 转到 45°
  setServoAngle(SERVO_CHANNEL_3, 90);  // 舵机 3 转到 90°
  setServoAngle(SERVO_CHANNEL_4, 135); // 舵机 4 转到 135°
  setServoAngle(SERVO_CHANNEL_5, 180); // 舵机 5 转到 180°
  setServoAngle(SERVO_CHANNEL_6, 90);  // 舵机 6 转到 90°
  setServoAngle(SERVO_CHANNEL_7, 45);  // 舵机 7 转到 45°
  setServoAngle(SERVO_CHANNEL_8, 0);   // 舵机 8 转到 0°
  delay(1000);

  setServoAngle(SERVO_CHANNEL_1, 90);  // 舵机 1 转到 90°
  setServoAngle(SERVO_CHANNEL_2, 135); // 舵机 2 转到 135°
  setServoAngle(SERVO_CHANNEL_3, 45);  // 舵机 3 转到 45°
  setServoAngle(SERVO_CHANNEL_4, 0);   // 舵机 4 转到 0°
  setServoAngle(SERVO_CHANNEL_5, 90);  // 舵机 5 转到 90°
  setServoAngle(SERVO_CHANNEL_6, 180); // 舵机 6 转到 180°
  setServoAngle(SERVO_CHANNEL_7, 135); // 舵机 7 转到 135°
  setServoAngle(SERVO_CHANNEL_8, 90);  // 舵机 8 转到 90°
  delay(1000);

  setServoAngle(SERVO_CHANNEL_1, 180); // 舵机 1 转到 180°
  setServoAngle(SERVO_CHANNEL_2, 90);  // 舵机 2 转到 90°
  setServoAngle(SERVO_CHANNEL_3, 0);   // 舵机 3 转到 0°
  setServoAngle(SERVO_CHANNEL_4, 90);  // 舵机 4 转到 45°
  setServoAngle(SERVO_CHANNEL_5, 0); // 舵机 5 转到 135°
  setServoAngle(SERVO_CHANNEL_6, 0);   // 舵机 6 转到 0°
  setServoAngle(SERVO_CHANNEL_7, 90);  // 舵机 7 转到 90°
  setServoAngle(SERVO_CHANNEL_8, 180); // 舵机 8 转到 180°
  delay(1000);
}
