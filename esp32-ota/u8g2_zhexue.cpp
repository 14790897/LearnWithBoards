#include <Arduino.h>
#include <secrets.h> // 包含 Wi-Fi 凭据
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <U8g2lib.h>

// 创建 HTTP 服务器
WebServer server(80);

// 初始化 SSD1306 屏幕
U8G2_SSD1306_128X64_NONAME_F_SW_I2C u8g2(U8G2_R0, /* clock=*/2, /* data=*/3, /* reset=*/U8X8_PIN_NONE);
IPAddress local_IP(192, 168, 0, 10); // 静态 IP 地址
IPAddress gateway(192, 168, 0, 1);   // 网关地址（通常是路由器地址）
IPAddress subnet(255, 255, 255, 0);  // 子网掩码
void handleUpload()
{
  // 添加 CORS 头部
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "POST, OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Content-Type");

  if (server.method() == HTTP_OPTIONS)
  {
    // 如果是 OPTIONS 请求，直接返回 200
    server.send(200);
    return;
  }

  if (server.method() != HTTP_POST)
  {
    server.send(405, "Method Not Allowed");
    return;
  }

  // 解析 JSON 数据
  StaticJsonDocument<1024> doc;
  DeserializationError error = deserializeJson(doc, server.arg("plain"));
  if (error)
  {
    server.send(400, "Bad Request", "JSON Parse Error");
    return;
  }

  // 获取帧数据
  JsonArray frame = doc["frame"];
  if (frame.isNull())
  {
    server.send(400, "Bad Request", "Missing 'frame' field");
    return;
  }

  // 将帧数据渲染到屏幕
  u8g2.clearBuffer();
  for (int y = 0; y < 64; y++)
  {
    for (int x = 0; x < 128; x++)
    {
      int index = y * 128 + x;
      if (frame[index] == 1)
      {
        u8g2.drawPixel(x, y);
      }
    }
  }
  u8g2.sendBuffer();

  server.send(200, "OK", "Frame received");
}

void setup()
{
  Serial.begin(115200);

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
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // 初始化屏幕
  u8g2.begin();

  // 设置 HTTP 路由
  server.on("/upload", handleUpload);
  server.begin();
  Serial.println("HTTP server started");
}

void loop()
{
  server.handleClient();
}
