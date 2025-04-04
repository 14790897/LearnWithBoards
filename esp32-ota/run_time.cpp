#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include "secrets.h"
#include <U8g2lib.h>

// U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/U8X8_PIN_NONE);
U8G2_SSD1306_128X64_NONAME_F_SW_I2C u8g2(U8G2_R0, /* clock=*/2, /* data=*/3, /* reset=*/U8X8_PIN_NONE);

IPAddress local_IP(192, 168, 0, 10); // 静态 IP 地址
IPAddress gateway(192, 168, 0, 1);    // 网关地址（通常是路由器地址）
IPAddress subnet(255, 255, 255, 0);   // 子网掩码

void setupOTA(const char* hostname) {
  ArduinoOTA.setHostname(hostname); // 设置设备名称
  // ArduinoOTA.setPassword("admin"); // 可选：设置 OTA 密码

  ArduinoOTA.onStart([]() {
    Serial.println("开始 OTA 更新...");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nOTA 更新完成!");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("进度: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("错误[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("认证失败");
    else if (error == OTA_BEGIN_ERROR) Serial.println("开始失败");
    else if (error == OTA_CONNECT_ERROR) Serial.println("连接失败");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("接收失败");
    else if (error == OTA_END_ERROR) Serial.println("结束失败");
  });

  ArduinoOTA.begin();
  Serial.println("OTA 已就绪");
}

void setup() {
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
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi 已连接");

  setupOTA("ESP32-OTA"); // 调用抽象的 OTA 配置函数
  Serial.print("IP 地址: ");
  Serial.println(WiFi.localIP());

  // 初始化屏幕
  u8g2.begin();
  u8g2.clearBuffer();                  // 清空屏幕缓冲区
  u8g2.setFont(u8g2_font_ncenB08_tr);  // 设置字体
  u8g2.drawStr(0, 10, "Hello, U8g2!"); // 在屏幕上显示文字
  u8g2.sendBuffer();                   // 将缓冲区内容发送到屏幕
}

void loop() {
  ArduinoOTA.handle(); // 处理 OTA 请求

  // 示例：屏幕动态显示时间
  // 动态显示运行时间
  u8g2.clearBuffer();                 // 清空屏幕缓冲区
  u8g2.setFont(u8g2_font_ncenB08_tr); // 设置字体
  u8g2.setCursor(0, 10);
  u8g2.print("Time: ");
  u8g2.print(millis() / 1000); // 显示运行时间（秒）
  u8g2.sendBuffer();           // 将缓冲区内容发送到屏幕
  delay(1000);                 // 每秒更新一次
}