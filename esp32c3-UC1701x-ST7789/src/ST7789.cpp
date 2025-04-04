#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include "secrets.h" // 包含 Wi-Fi 凭据（ssid 和 password）
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>

// ST7789V 引脚定义
#define TFT_CS 7   // 片选引脚
#define TFT_DC 6   // 数据/命令引脚
#define TFT_RST 10 // 复位引脚

// 创建 ST7789 对象
Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);

// 创建 HTTP 服务器
WebServer server(80);

// 静态 IP 配置
IPAddress local_IP(192, 168, 0, 10); // 静态 IP 地址
IPAddress gateway(192, 168, 0, 1);   // 网关地址
IPAddress subnet(255, 255, 255, 0);  // 子网掩码

// 是否显示帧数据的标志
bool displayFrame = false;
uint8_t frameBuffer[240 * 240]; // ST7789 240x240 的帧缓冲区

// 处理上传的帧数据
void handleUpload()
{
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.sendHeader("Access-Control-Allow-Methods", "POST, OPTIONS");
    server.sendHeader("Access-Control-Allow-Headers", "Content-Type");

    if (server.method() == HTTP_OPTIONS)
    {
        server.send(200);
        return;
    }

    if (server.method() != HTTP_POST)
    {
        server.send(405, "Method Not Allowed");
        return;
    }

    StaticJsonDocument<1024> doc;
    DeserializationError error = deserializeJson(doc, server.arg("plain"));
    if (error)
    {
        server.send(400, "Bad Request", "JSON Parse Error");
        return;
    }

    JsonArray frame = doc["frame"];
    if (frame.isNull())
    {
        server.send(400, "Bad Request", "Missing 'frame' field");
        return;
    }

    // 将接收到的帧数据存入缓冲区（假设 frame 是 240x240 的二值数组）
    for (int i = 0; i < 240 * 240 && i < frame.size(); i++)
    {
        frameBuffer[i] = frame[i] == 1 ? 1 : 0;
    }
    displayFrame = true; // 设置标志以显示帧数据

    server.send(200, "OK", "Frame received");
}

namespace ST7789
{
    void setup()
    {
        // 初始化串口调试
        Serial.begin(115200);
        Serial.println("Adafruit ST7789 初始化...");

        // 初始化 ST7789 显示屏
        tft.init(240, 240);
        tft.setRotation(1);
        tft.fillScreen(ST77XX_BLACK);

        // 显示初始化文字
        tft.setTextColor(ST77XX_WHITE);
        tft.setTextSize(2);
        tft.setCursor(10, 30);
        tft.print("Connecting WiFi...");

        // 配置静态 IP 和 WiFi
        if (!WiFi.config(local_IP, gateway, subnet))
        {
            Serial.println("静态 IP 配置失败！");
        }
        WiFi.mode(WIFI_STA);
        WiFi.setHostname("ESP32_C3_123");
        WiFi.begin(ssid, password);
        while (WiFi.status() != WL_CONNECTED)
        {
            delay(500);
            Serial.print(".");
        }
        Serial.println("WiFi connected");
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());

        // 更新屏幕显示 IP
        tft.fillScreen(ST77XX_BLACK);
        tft.setCursor(10, 30);
        tft.print("IP: ");
        tft.print(WiFi.localIP());

        // 设置 HTTP 路由
        server.on("/upload", handleUpload);
        server.begin();
        Serial.println("HTTP server started");
    }

    void ST7789_loop()
    {
        // 处理 HTTP 请求
        server.handleClient();

        // 如果收到帧数据，则显示
        if (displayFrame)
        {
            tft.fillScreen(ST77XX_BLACK);
            for (int y = 0; y < 240; y++)
            {
                for (int x = 0; x < 240; x++)
                {
                    if (frameBuffer[y * 240 + x] == 1)
                    {
                        tft.drawPixel(x, y, ST77XX_WHITE); // 显示白色像素
                    }
                }
            }
            displayFrame = false; // 重置标志
        }
        else
        {
            // 默认动画：移动矩形
            static int x = 0, y = 0;
            static int dx = 5, dy = 5;
            static int rectWidth = 50, rectHeight = 50;
            static uint16_t colors[] = {ST77XX_RED, ST77XX_GREEN, ST77XX_BLUE, ST77XX_YELLOW, ST77XX_CYAN, ST77XX_MAGENTA};
            static int colorIndex = 0;
            static unsigned long lastUpdate = 0;

            unsigned long now = millis();
            if (now - lastUpdate >= 50) // 每 50ms 更新一次
            {
                tft.fillScreen(ST77XX_BLACK);
                tft.fillRect(x, y, rectWidth, rectHeight, colors[colorIndex]);

                x += dx;
                y += dy;

                if (x <= 0 || x + rectWidth >= 240)
                {
                    dx = -dx;
                    colorIndex = (colorIndex + 1) % 6;
                }
                if (y <= 0 || y + rectHeight >= 240) // ST7789 是 240x240，修正边界
                {
                    dy = -dy;
                    colorIndex = (colorIndex + 1) % 6;
                }

                lastUpdate = now;
            }
        }
    }
}

