#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include <time.h> // Include time library for NTP
#include "../include/secrets.h" // 包含设备配置和敏感信息

// 创建 Web 服务器，端口 80
WebServer server(80);

// LED 状态变量
bool ledState = false;
int ledBrightness = 255; // LED 亮度 (0-255)

// 设备配置结构
// struct DeviceConfig is already defined elsewhere, so this duplicate definition is removed.

// 全局配置对象
DeviceConfig config;

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

// LED 控制函数
void setLED(bool state, int brightness = 255) {
  ledState = state;
  ledBrightness = brightness;

  if (state) {
    analogWrite(LED_PIN, brightness);
  } else {
    digitalWrite(LED_PIN, LOW);
  }
}

// 获取 LED 状态
String getLEDState() {
  StaticJsonDocument<128> doc;
  doc["state"] = ledState;
  doc["brightness"] = ledBrightness;

  String response;
  serializeJson(doc, response);
  return response;
}

// 保存配置的前向声明
bool saveConfig();

// 加载配置
bool loadConfig() {
  // 检查配置文件是否存在
  if (!LittleFS.exists("/config.json")) {
    Serial.println("未找到配置文件，使用默认设置");

    // 设置默认配置
    strlcpy(config.hostname, DEFAULT_HOSTNAME, sizeof(config.hostname));
    strlcpy(config.wifi_ssid, DEFAULT_SSID, sizeof(config.wifi_ssid));
    strlcpy(config.wifi_password, DEFAULT_PASSWORD, sizeof(config.wifi_password));
    config.dhcp_enabled = false;
    config.static_ip = DEFAULT_IP;
    config.gateway = DEFAULT_GATEWAY;
    config.subnet = DEFAULT_SUBNET;
    config.motor_speed = DEFAULT_MOTOR_SPEED;

    // 保存默认配置
    return saveConfig();
  }

  // 打开配置文件
  File configFile = LittleFS.open("/config.json", "r");
  if (!configFile) {
    Serial.println("无法打开配置文件");
    return false;
  }

  // 解析 JSON
  StaticJsonDocument<1024> doc;
  DeserializationError error = deserializeJson(doc, configFile);
  configFile.close();

  if (error) {
    Serial.print("解析配置文件失败: ");
    Serial.println(error.c_str());
    return false;
  }

  // 复制配置到结构体
  strlcpy(config.hostname, doc["hostname"] | DEFAULT_HOSTNAME, sizeof(config.hostname));
  strlcpy(config.wifi_ssid, doc["wifi_ssid"] | DEFAULT_SSID, sizeof(config.wifi_ssid));
  strlcpy(config.wifi_password, doc["wifi_password"] | DEFAULT_PASSWORD, sizeof(config.wifi_password));
  config.dhcp_enabled = doc["dhcp_enabled"] | false;

  // 解析 IP 地址
  String ip = doc["static_ip"] | DEFAULT_IP.toString();
  String gw = doc["gateway"] | DEFAULT_GATEWAY.toString();
  String sn = doc["subnet"] | DEFAULT_SUBNET.toString();
  config.static_ip.fromString(ip);
  config.gateway.fromString(gw);
  config.subnet.fromString(sn);

  config.motor_speed = doc["motor_speed"] | 255;

  Serial.println("配置加载成功");
  return true;
}

// 保存配置
bool saveConfig() {
  StaticJsonDocument<1024> doc;

  // 将配置复制到 JSON 文档
  doc["hostname"] = config.hostname;
  doc["wifi_ssid"] = config.wifi_ssid;
  doc["wifi_password"] = config.wifi_password;
  doc["dhcp_enabled"] = config.dhcp_enabled;
  doc["static_ip"] = config.static_ip.toString();
  doc["gateway"] = config.gateway.toString();
  doc["subnet"] = config.subnet.toString();
  doc["motor_speed"] = config.motor_speed;

  // 打开文件进行写入
  File configFile = LittleFS.open("/config.json", "w");
  if (!configFile) {
    Serial.println("无法打开配置文件进行写入");
    return false;
  }

  // 写入 JSON 到文件
  if (serializeJson(doc, configFile) == 0) {
    Serial.println("写入配置文件失败");
    configFile.close();
    return false;
  }

  configFile.close();
  Serial.println("配置保存成功");
  return true;
}

// 获取设备配置
String getDeviceConfig() {
  StaticJsonDocument<1024> doc;

  doc["hostname"] = config.hostname;
  doc["wifi_ssid"] = config.wifi_ssid;
  // 出于安全考虑，不返回密码
  doc["wifi_password"] = "********";
  doc["dhcp_enabled"] = config.dhcp_enabled;
  doc["static_ip"] = config.static_ip.toString();
  doc["gateway"] = config.gateway.toString();
  doc["subnet"] = config.subnet.toString();
  doc["motor_speed"] = config.motor_speed;

  // 添加当前设备信息
  doc["current_ip"] = WiFi.localIP().toString();
  doc["mac_address"] = WiFi.macAddress();
  doc["rssi"] = WiFi.RSSI();
  doc["firmware_version"] = "1.0.0";

  String response;
  serializeJson(doc, response);
  return response;
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


// 处理静态文件
void handleFileRead(String path) {
  if (path.endsWith("/")) {
    path += "index.html";
  }

  String contentType;
  if (path.endsWith(".html")) {
    contentType = "text/html";
  } else if (path.endsWith(".css")) {
    contentType = "text/css";
  } else if (path.endsWith(".js")) {
    contentType = "application/javascript";
  } else if (path.endsWith(".json")) {
    contentType = "application/json";
  } else {
    contentType = "text/plain";
  }

  if (LittleFS.exists(path)) {
    File file = LittleFS.open(path, "r");
    server.streamFile(file, contentType);
    file.close();
  } else {
    server.send(404, "text/plain", "File Not Found");
  }
}

// 处理 LED 状态获取
void handleLEDGet() {
  server.send(200, "application/json", getLEDState());
}

// 处理 LED 状态设置
void handleLEDPost() {
  bool newState = ledState;
  int newBrightness = ledBrightness;

  for (int i = 0; i < server.args(); i++) {
    if (server.argName(i) == "state") {
      newState = (server.arg(i) == "true" || server.arg(i) == "1");
    } else if (server.argName(i) == "brightness") {
      newBrightness = server.arg(i).toInt();
    }
  }

  setLED(newState, newBrightness);
  server.send(200, "application/json", getLEDState());
}

// 处理配置获取
void handleConfigGet() {
  server.send(200, "application/json", getDeviceConfig());
}

// 处理配置保存
void handleConfigPost() {
  bool configChanged = false;
  bool wifiChanged = false;
  bool hostnameChanged = false;

  for (int i = 0; i < server.args(); i++) {
    String argName = server.argName(i);
    String argValue = server.arg(i);

    if (argName == "hostname" && argValue.length() > 0) {
      strlcpy(config.hostname, argValue.c_str(), sizeof(config.hostname));
      configChanged = true;
      hostnameChanged = true;
    }
    else if (argName == "wifi_ssid" && argValue.length() > 0) {
      strlcpy(config.wifi_ssid, argValue.c_str(), sizeof(config.wifi_ssid));
      configChanged = true;
      wifiChanged = true;
    }
    else if (argName == "wifi_password" && argValue.length() > 0 && argValue != "********") {
      strlcpy(config.wifi_password, argValue.c_str(), sizeof(config.wifi_password));
      configChanged = true;
      wifiChanged = true;
    }
    else if (argName == "dhcp_enabled") {
      config.dhcp_enabled = (argValue == "true" || argValue == "1");
      configChanged = true;
    }
    else if (argName == "static_ip" && argValue.length() > 0) {
      IPAddress ip;
      if (ip.fromString(argValue.c_str())) {
        config.static_ip = ip;
        configChanged = true;
      }
    }
    else if (argName == "gateway" && argValue.length() > 0) {
      IPAddress gw;
      if (gw.fromString(argValue.c_str())) {
        config.gateway = gw;
        configChanged = true;
      }
    }
    else if (argName == "subnet" && argValue.length() > 0) {
      IPAddress sn;
      if (sn.fromString(argValue.c_str())) {
        config.subnet = sn;
        configChanged = true;
      }
    }
    else if (argName == "motor_speed") {
      int speed = argValue.toInt();
      if (speed >= 0 && speed <= 255) {
        config.motor_speed = speed;
        configChanged = true;
      }
    }
  }

  // 如果配置发生变化，保存配置
  if (configChanged) {
    saveConfig();

    // 如果主机名变化，更新 OTA 主机名
    if (hostnameChanged) {
      ArduinoOTA.setHostname(config.hostname);
    }

    // 返回成功响应
    StaticJsonDocument<256> doc;
    doc["success"] = true;
    doc["message"] = "配置已保存";

    // 如果 WiFi 设置变化，提示需要重启
    if (wifiChanged) {
      doc["restart_required"] = true;
      doc["message"] = "配置已保存，需要重启设备以应用 WiFi 设置";
    } else {
      doc["restart_required"] = false;
    }

    String response;
    serializeJson(doc, response);
    server.send(200, "application/json", response);
  } else {
    // 没有变化，返回错误
    server.send(400, "application/json", "{\"success\":false,\"message\":\"没有有效的参数变化\"}");
  }
}

// 处理设备重启
void handleRestart() {
  server.send(200, "application/json", "{\"success\":true,\"message\":\"设备将在 3 秒后重启\"}");
  delay(3000);
  ESP.restart();
}

// 设置 Web 服务器路由
void setupWebServer() {
  // 静态文件处理
  server.on("/", HTTP_GET, []() {
    handleFileRead("/index.html");
  });

  server.on("/style.css", HTTP_GET, []() {
    handleFileRead("/style.css");
  });

  server.on("/script.js", HTTP_GET, []() {
    handleFileRead("/script.js");
  });

  // API 端点
  server.on("/api/led", HTTP_GET, handleLEDGet);
  server.on("/api/led", HTTP_POST, handleLEDPost);
  server.on("/api/config", HTTP_GET, handleConfigGet);
  server.on("/api/config", HTTP_POST, handleConfigPost);
  server.on("/api/restart", HTTP_POST, handleRestart);

  // 未找到页面处理
  server.onNotFound([]() {
    handleFileRead(server.uri());
  });

  // 启动服务器
  server.begin();
  Serial.println("HTTP 服务器已启动");
}

void setup()
{
  // 初始化串口（用于调试）
  Serial.begin(115200);
  Serial.println("ESP32-C3 启动中...");

  // 初始化 LittleFS
  if (!LittleFS.begin(true)) {
    Serial.println("LittleFS 初始化失败");
    return;
  }
  Serial.println("LittleFS 初始化成功");

  // 加载配置
  if (!loadConfig()) {
    Serial.println("加载配置失败，使用默认设置");
  }

  // 设置 WiFi 模式
  WiFi.mode(WIFI_STA);

  // 设置主机名
  WiFi.setHostname(config.hostname);
  Serial.print("主机名: ");
  Serial.println(config.hostname);

  // 配置 IP 地址
  if (!config.dhcp_enabled) {
    if (!WiFi.config(config.static_ip, config.gateway, config.subnet)) {
      Serial.println("静态 IP 配置失败！");
    } else {
      Serial.print("静态 IP 地址: ");
      Serial.println(config.static_ip.toString());
    }
  } else {
    Serial.println("使用 DHCP 获取 IP 地址");
  }

  // 连接 WiFi
  Serial.print("连接到 WiFi: ");
  Serial.print(config.wifi_ssid);
  Serial.print(" ");

  WiFi.begin(config.wifi_ssid, config.wifi_password);
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) // 最多尝试 20 次，大约 10 秒
  {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi 已连接");
  } else {
    Serial.println("\nWiFi 连接失败，将使用默认设置重试");
    // 如果使用配置中的凭证连接失败，尝试使用默认凭证
    WiFi.begin(DEFAULT_SSID, DEFAULT_PASSWORD);
    while (WiFi.status() != WL_CONNECTED)
    {
      delay(500);
      Serial.print(".");
    }
    Serial.println("\nWiFi 已连接（使用默认凭证）");
  }

  setupOTA(config.hostname); // 调用抽象的 OTA 配置函数
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

  // 初始化 LED 引脚
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  // 设置 Web 服务器
  setupWebServer();

  Serial.println("Motor Control Start");
}

void loop()
{
  ArduinoOTA.handle(); // 处理 OTA 请求
  server.handleClient(); // 处理 Web 请求

  // 使用配置中的电机速度
  int speed = config.motor_speed; // 从配置中获取电机速度

  // 四个电机持续正转
  // Serial.println("Motors Running Forward"); // 注释掉以减少串口输出
  setMotor(MOTOR1_IA, MOTOR1_IB, speed);       // 正常电机
  setMotor(MOTOR2_IA, MOTOR2_IB, speed, true); // 反向电机
  setMotor(MOTOR3_IA, MOTOR3_IB, speed);       // 正常电机
  setMotor(MOTOR4_IA, MOTOR4_IB, speed, true); // 反向电机

  delay(10); // 短暂延迟，但不要太长，以便及时响应 Web 请求
}