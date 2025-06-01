#include <Arduino.h>
#include <ESP32Servo.h>


#define TRIG_PIN 3 // 超声波传感器触发引脚
#define ECHO_PIN 10
#define SERVO_PIN 9 // 舵机控制引脚

Servo myServo;       
int scanMin = 0;      // 扫描起始角度
int scanMax = 180;    // 扫描结束角度
int scanStep = 5;     // 扫描步进角度
int currentAngle = 90; // 当前舵机角度
int step = 20;          // 舵机每次调整步长，越小跟踪越平滑

float measureDistance()
{
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  long duration = pulseIn(ECHO_PIN, HIGH, 30000);
  Serial.print("pulseIn duration: ");
  Serial.println(duration);
  if (duration == 0)
    return -1;
  return duration * 0.034 / 2;
}
float measureAtAngle(int angle)
{
  myServo.write(angle);
  delay(100); // 舵机稳定
  return measureDistance();
}



void setup()
{
  Serial.begin(115200);
  Serial.println("Start");
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  myServo.attach(SERVO_PIN);
  myServo.write(currentAngle);
}


void loop()
{
  static float lastLeftDist = 9999;
  static float lastRightDist = 9999;

  float leftDist = (currentAngle - step >= 0) ? measureAtAngle(currentAngle - step) : 9999;
  delay(100); // 等待舵机稳定
  float rightDist = (currentAngle + step <= 180) ? measureAtAngle(currentAngle + step) : 9999;

  // 只检测距离20以内的目标
  const float maxDetectDist = 20;
  if (leftDist > maxDetectDist || leftDist < 0) leftDist = 9999;
  if (rightDist > maxDetectDist || rightDist < 0) rightDist = 9999;

  // 如果本次没检测到就用上次的数据
  // if (leftDist == 9999) leftDist = lastLeftDist;
  // else lastLeftDist = leftDist;
  // if (rightDist == 9999) rightDist = lastRightDist;
  // else lastRightDist = rightDist;

  // 只根据左右距离判断
  if (leftDist < rightDist)
  {
    currentAngle = max(0, currentAngle - step);
    myServo.write(currentAngle);
  }
  else if (rightDist < leftDist)
  {
    currentAngle = min(180, currentAngle + step);
    myServo.write(currentAngle);
  }
  else
  {
    // 保持当前位置
    myServo.write(currentAngle);
  }

  Serial.print("当前角度: ");
  Serial.print(currentAngle);
  Serial.print(" 左: ");
  Serial.print(leftDist);
  Serial.print(" 右: ");
  Serial.println(rightDist);

  delay(100);
}

