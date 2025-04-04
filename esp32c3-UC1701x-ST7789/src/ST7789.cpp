#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include "secrets.h"
#include <WiFi.h>
#include <WebSocketsServer.h>

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
void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length)
{
    if (type == WStype_BIN && length == 7200)
    {
        for (int i = 0; i < length; i++)
        {
            uint8_t byte = payload[i];
            for (int bit = 0; bit < 8; bit++)
            {
                frameBuffer[i * 8 + (7 - bit)] = (byte >> bit) & 1;
            }
        }
        displayFrame = true;
    }
}

namespace ST7789
{
    void setup()
    {
        Serial.begin(115200);
        Serial.println("Adafruit ST7789 初始化...");

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

    void ST7789_loop()
    {
        webSocket.loop();

        if (displayFrame)
        {
            for (int i = 0; i < 240 * 240; i++)
            {
                pixelBuffer[i] = frameBuffer[i] ? ST77XX_WHITE : ST77XX_BLACK;
            }
            tft.startWrite();
            tft.setAddrWindow(0, 0, 240, 240);
            tft.writePixels(pixelBuffer, 240 * 240);
            tft.endWrite();
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

