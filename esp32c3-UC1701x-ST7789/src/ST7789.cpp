#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include "secrets.h"
#include <WiFi.h>
#include <WebSocketsServer.h>
#include "DiagnosticMode.h"

#define TFT_CS 7
#define TFT_DC 6
#define TFT_RST 10

Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);
WebSocketsServer webSocket = WebSocketsServer(80);

IPAddress local_IP(192, 168, 0, 10);
IPAddress gateway(192, 168, 0, 1);
IPAddress subnet(255, 255, 255, 0);

bool displayFrame = false;
static uint8_t frameBuffer[240 * 240]; // Changed to static

// Pre-allocate the pixel buffer as static
static uint16_t pixelBuffer[240 * 240];
// 添加帧处理计数器和时间统计
static unsigned long lastFrameTime = 0;
static int frameCount = 0;
static int droppedFrames = 0;

// 外部变量声明给DiagnosticMode.cpp使用
extern bool frameRateLimitEnabled;

void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length)
{
    if (type == WStype_BIN && length == 7200)
    {
        unsigned long currentTime = millis();

        // 如果上一帧还没处理完，则丢弃这一帧
        if (displayFrame)
        {
            droppedFrames++;
            // 每10帧打印一次丢帧统计
            if (droppedFrames % 10 == 0)
            {
                Serial.printf("丢弃的帧数: %d (帧率限制: %s)\n",
                              droppedFrames,
                              frameRateLimitEnabled ? "开启" : "关闭");
            }
            // 发送ACK以便前端知道需要减慢速度
            webSocket.sendTXT(num, "ACK");
            return;
        }

        // 如果帧率限制关闭或者上一帧已处理完毕，则处理新帧
        // 处理帧数据
        for (int i = 0; i < length; i++)
        {
            uint8_t byte = payload[i];
            for (int bit = 0; bit < 8; bit++)
            {
                frameBuffer[i * 8 + (7 - bit)] = (byte >> bit) & 1; // 这里也可以使用或0
            }
        }

        // 计算帧率
        frameCount++;
        if (currentTime - lastFrameTime >= 5000)
        { // 每5秒打印一次帧率
            float fps = frameCount / ((currentTime - lastFrameTime) / 1000.0f);
            Serial.printf("FPS: %.2f, 丢帧: %d, 帧率限制: %s\n",
                          fps, droppedFrames,
                          frameRateLimitEnabled ? "开启" : "关闭");
            frameCount = 0;
            lastFrameTime = currentTime;
        }

        displayFrame = true;

        // 发送确认消息
        webSocket.sendTXT(num, "ACK");
    }
    else if (type == WStype_CONNECTED)
    {
        Serial.printf("WebSocket客户端 %u 已连接\n", num);
    }
    else if (type == WStype_DISCONNECTED)
    {
        Serial.printf("WebSocket客户端 %u 已断开连接\n", num);
    }
}

namespace ST7789
{
    void setup()
    {
        Serial.begin(115200);
        Serial.println("Adafruit ST7789 初始化...");
        Serial.println("输入 'diag' 进入诊断模式");

        tft.init(240, 240);
        tft.setRotation(1);
        tft.fillScreen(ST77XX_BLACK);

        tft.setTextColor(ST77XX_WHITE);
        tft.setTextSize(2);
        tft.setCursor(10, 30);
        tft.print("Connecting WiFi...");

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

        tft.fillScreen(ST77XX_BLACK);
        tft.setCursor(10, 30);
        tft.print("IP: ");
        tft.print(WiFi.localIP());

        webSocket.begin();
        webSocket.onEvent(webSocketEvent);
        Serial.println("WebSocket server started");
    }

    // 添加WiFi监控函数
    void checkWiFiConnection()
    {
        static unsigned long lastWiFiCheck = 0;
        unsigned long currentMillis = millis();

        // 每10秒检查一次WiFi连接
        if (currentMillis - lastWiFiCheck >= 10000)
        {
            lastWiFiCheck = currentMillis;

            if (WiFi.status() != WL_CONNECTED)
            {
                Serial.println("WiFi连接已断开，尝试重新连接...");

                // 显示重连信息
                tft.fillScreen(ST77XX_BLACK);
                tft.setCursor(10, 30);
                tft.setTextColor(ST77XX_RED);
                tft.print("WiFi Reconnecting...");

                // 尝试重新连接
                WiFi.reconnect();

                // 等待重新连接
                int attempts = 0;
                while (WiFi.status() != WL_CONNECTED && attempts < 20)
                {
                    delay(500);
                    Serial.print(".");
                    attempts++;
                }

                if (WiFi.status() == WL_CONNECTED)
                {
                    Serial.println("\nWiFi重新连接成功");
                    Serial.print("IP地址: ");
                    Serial.println(WiFi.localIP());

                    // 显示新的IP地址
                    tft.fillScreen(ST77XX_BLACK);
                    tft.setCursor(10, 30);
                    tft.setTextColor(ST77XX_WHITE);
                    tft.print("IP: ");
                    tft.print(WiFi.localIP());
                }
                else
                {
                    Serial.println("\nWiFi重新连接失败");
                }
            }
        }
    }

    void ST7789_loop()
    {
        // 处理串口输入，检测是否进入诊断模式
        if (Serial.available() && !diagnosticModeEnabled)
        {
            String input = Serial.readStringUntil('\n');
            input.trim();
            if (input == "diag" || input == "diagnostic")
            {
                enterDiagnosticMode();
            }
        }

        // 如果在诊断模式中，处理诊断命令
        if (diagnosticModeEnabled)
        {
            processDiagnosticCommands();
        }

        // 检查WiFi连接
        checkWiFiConnection();

        // 处理WebSocket消息
        webSocket.loop();

        // 如果有新帧需要显示
        if (displayFrame)
        {
            unsigned long startTime = millis();

            // 将二值帧数据转换为像素颜色
            for (int i = 0; i < 240 * 240; i++)
            {
                pixelBuffer[i] = frameBuffer[i] ? ST77XX_WHITE : ST77XX_BLACK;
            }

            // 使用批量写入方式更新显示
            tft.startWrite();
            tft.setAddrWindow(0, 0, 240, 240);
            tft.writePixels(pixelBuffer, 240 * 240);
            tft.endWrite();

            // 计算并打印帧处理时间
            unsigned long processTime = millis() - startTime;
            if (processTime > 100)
            { // 只打印处理时间较长的帧
                Serial.printf("帧处理时间: %lu ms\n", processTime);
            }

            displayFrame = false;
        }
        else
        {
            // static int x = 0, y = 0;
            // static int dx = 5, dy = 5;
            // static int rectWidth = 50, rectHeight = 50;
            // static uint16_t colors[] = {ST77XX_RED, ST77XX_GREEN, ST77XX_BLUE, ST77XX_YELLOW, ST77XX_CYAN, ST77XX_MAGENTA};
            // static int colorIndex = 0;
            // static unsigned long lastUpdate = 0;

            // unsigned long now = millis();
            // if (now - lastUpdate >= 50)
            // {
            //     tft.fillScreen(ST77XX_BLACK);
            //     tft.fillRect(x, y, rectWidth, rectHeight, colors[colorIndex]);

            //     x += dx;
            //     y += dy;

            //     if (x <= 0 || x + rectWidth >= 240)
            //     {
            //         dx = -dx;
            //         colorIndex = (colorIndex + 1) % 6;
            //     }
            //     if (y <= 0 || y + rectHeight >= 240)
            //     {
            //         dy = -dy;
            //         colorIndex = (colorIndex + 1) % 6;
            //     }
            //     lastUpdate = now;
            // }
        }
    }
}

