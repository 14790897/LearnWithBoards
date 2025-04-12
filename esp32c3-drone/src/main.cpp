#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include "secrets.h"
#include <time.h> // Include time library for NTP
// 定义引脚
#define MOTOR1_IA 11                 // 电机 1 的 IA (PWM 引脚)
#define MOTOR1_IB 7                  // 电机 1 的 IB
#define MOTOR2_IA 4                  // 电机 2 的 IA (PWM 引脚)
#define MOTOR2_IB 5                  // 电机 2 的 IB
#define MOTOR3_IA 3                  // 电机 3 的 IA (PWM 引脚)
#define MOTOR3_IB 2                  // 电机 3 的 IB
#define MOTOR4_IA 6                  // 电机 4 的 IA (PWM 引脚)
#define MOTOR4_IB 10                 // 电机 4 的 IB
IPAddress local_IP(192, 168, 0, 10); // 静态 IP 地址
IPAddress gateway(192, 168, 0, 1);   // 网关地址（通常是路由器地址）
IPAddress subnet(255, 255, 255, 0);  // 子网掩码

void setupOTA(const char *hostname)
{
  ArduinoOTA.setHostname(hostname); // 设置设备名称
  // ArduinoOTA.setPassword("admin"); // 可选：设置 OTA 密码

  ArduinoOTA.onStart([]()
                     { Serial.println("开始 OTA 更新..."); });
  ArduinoOTA.onEnd([]()
                   { Serial.println("\nOTA 更新完成!"); });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total)
                        { Serial.printf("进度: %u%%\r", (progress / (total / 100))); });
  ArduinoOTA.onError([](ota_error_t error)
                     {
    Serial.printf("错误[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("认证失败");
    else if (error == OTA_BEGIN_ERROR) Serial.println("开始失败");
    else if (error == OTA_CONNECT_ERROR) Serial.println("连接失败");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("接收失败");
    else if (error == OTA_END_ERROR) Serial.println("结束失败"); });

  ArduinoOTA.begin();
  Serial.println("OTA 已就绪");
}

// 电机控制函数
void setMotor(int iaPin, int ibPin, int speed, bool isReversed = false)
{
  if (isReversed)
  {
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


void setup()
{
  // 配置静态 IP 地址
  if (!WiFi.config(local_IP, gateway, subnet))
  {
    Serial.println("静态 IP 配置失败！");
  }
  WiFi.mode(WIFI_STA);
  WiFi.setHostname("ESP32_C3_123"); // 设置设备名称
  Serial.print("ESP32_C3_123: ");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi 已连接");

  setupOTA("ESP32-OTA"); // 调用抽象的 OTA 配置函数
  Serial.print("IP 地址: ");
  Serial.println(WiFi.localIP());

  // 初始化电机引脚
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

void loop()
{
  ArduinoOTA.handle(); // 处理 OTA 请求

  // 设置 PWM 值为 189 (5V * 0.74 ≈ 3.7V)
  int speed = 255; // 74% 占空比，适合 3.7V 电机

  // 四个电机持续正转
  Serial.println("Motors Running Forward");
  setMotor(MOTOR1_IA, MOTOR1_IB, speed);       // 正常电机
  setMotor(MOTOR2_IA, MOTOR2_IB, speed, true); // 反向电机
  setMotor(MOTOR3_IA, MOTOR3_IB, speed);       // 正常电机
  setMotor(MOTOR4_IA, MOTOR4_IB, speed, true); // 反向电机

  delay(100); // 短暂延迟以避免串口输出过快
}