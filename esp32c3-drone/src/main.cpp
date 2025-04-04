#include <Arduino.h>
// 定义引脚
#define MOTOR1_IA 11  // 电机 1 的 IA (PWM 引脚)
#define MOTOR1_IB 7 // 电机 1 的 IB
#define MOTOR2_IA 4  // 电机 2 的 IA (PWM 引脚)
#define MOTOR2_IB 5  // 电机 2 的 IB
#define MOTOR3_IA 3  // 电机 3 的 IA (PWM 引脚)
#define MOTOR3_IB 2  // 电机 3 的 IB
#define MOTOR4_IA 6  // 电机 4 的 IA (PWM 引脚)
#define MOTOR4_IB 10 // 电机 4 的 IB
void setup()
{
  // 设置引脚为输出模式
  pinMode(MOTOR1_IA, OUTPUT);
  pinMode(MOTOR1_IB, OUTPUT);
  pinMode(MOTOR2_IA, OUTPUT);
  pinMode(MOTOR2_IB, OUTPUT);
  pinMode(MOTOR3_IA, OUTPUT);
  pinMode(MOTOR3_IB, OUTPUT);
  pinMode(MOTOR4_IA, OUTPUT);
  pinMode(MOTOR4_IB, OUTPUT);

  // 初始化串口（用于调试，可选）
  Serial.begin(115200);
  Serial.println("Motor Control Start");
}

// 电机控制函数
void setMotor(int iaPin, int ibPin, int speed, bool isReversed = false)
{
  if (isReversed) {
    speed = -speed; // 如果是反向电机，反转速度方向
  }

  if (speed > 0)
  {                            // 正转
    analogWrite(iaPin, speed); // PWM 控制速度
    digitalWrite(ibPin, LOW);
  }
  else if (speed < 0)
  { // 反转
    analogWrite(iaPin, 0);
    digitalWrite(ibPin, HIGH); // 注意：反转时 IB 不支持 PWM
  }
  else
  { // 停止
    digitalWrite(iaPin, LOW);
    digitalWrite(ibPin, LOW);
  }
}

void loop()
{ 
  // 设置 PWM 值为 189 (5V * 0.74 ≈ 3.7V)
  int speed = 255; // 74% 占空比，适合 3.7V 电机

  // 四个电机持续正转
  Serial.println("Motors Running Forward");
  setMotor(MOTOR1_IA, MOTOR1_IB, speed);          // 正常电机
  setMotor(MOTOR2_IA, MOTOR2_IB, speed, true);    // 反向电机
  setMotor(MOTOR3_IA, MOTOR3_IB, speed);          // 正常电机
  setMotor(MOTOR4_IA, MOTOR4_IB, speed, true);    // 反向电机

  delay(100); // 短暂延迟以避免串口输出过快
}