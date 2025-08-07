#include<WiFi.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include "secrets.h"
#include <time.h> // Include time library for NTP

IPAddress local_IP(192, 168, 0, 220); // 静态 IP 地址
IPAddress gateway(192, 168, 0, 1);   // 网关地址（通常是路由器地址）
IPAddress subnet(255, 255, 255, 0);  // 子网掩码

void setupOTA(const char *hostname)
{
  ArduinoOTA.setPort(3232);         // 设置 OTA 端口为 3232
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

void setup()
{
  Serial.begin(115200);
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

  // 初始化 mDNS
  if (!MDNS.begin("ESP32_C3_123"))
  {
    Serial.println("mDNS 启动失败!");
  }
  else
  {
    Serial.println("mDNS 已启动: ESP32_C3_123.local");
    MDNS.addService("arduino123", "tcp", 3232); // 添加 OTA 服务端口 3232
  }

  setupOTA("ESP32_C3_123"); // 使用与hostname一致的名称
  Serial.print("IP 地址: ");
  Serial.println(WiFi.localIP());
  Serial.println("OTA 服务已启动在端口 3232");
}

void loop()
{
  ArduinoOTA.handle(); // 处理 OTA 请求

  delay(100); // 减少延时以提高响应性
}
