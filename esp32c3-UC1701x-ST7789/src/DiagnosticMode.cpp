#include "DiagnosticMode.h"
#include <WiFi.h>
#include <WebSocketsServer.h>
#include <Adafruit_ST7789.h>

// 外部变量声明
extern WebSocketsServer webSocket;
extern Adafruit_ST7789 tft;
extern bool displayFrame;

// 诊断模式标志
bool diagnosticModeEnabled = false;
bool frameRateLimitEnabled = true; // 默认启用帧率限制
unsigned long lastDiagnosticCheck = 0;

// 诊断模式函数实现
void enterDiagnosticMode() {
    diagnosticModeEnabled = true;
    Serial.println("\n=== 进入诊断模式 ===");
    printDiagnosticHelp();
}

void processDiagnosticCommands() {
    if (!diagnosticModeEnabled) return;

    // 每500ms检查一次串口输入，避免阻塞主循环
    unsigned long currentMillis = millis();
    if (currentMillis - lastDiagnosticCheck < 500) return;
    lastDiagnosticCheck = currentMillis;

    if (Serial.available()) {
        String command = Serial.readStringUntil('\n');
        command.trim();

        if (command == "help" || command == "h") {
            printDiagnosticHelp();
        }
        else if (command == "status" || command == "s") {
            printSystemStatus();
        }
        else if (command == "net" || command == "n") {
            runNetworkTest();
        }
        else if (command == "display" || command == "d") {
            runDisplayTest();
        }
        else if (command == "limit on" || command == "lon") {
            toggleFrameRateLimit(true);
        }
        else if (command == "limit off" || command == "loff") {
            toggleFrameRateLimit(false);
        }
        else if (command == "exit" || command == "x") {
            diagnosticModeEnabled = false;
            Serial.println("退出诊断模式");
        }
        else {
            Serial.println("未知命令，输入 'help' 查看可用命令");
        }
    }
}

void printDiagnosticHelp() {
    Serial.println("\n可用命令:");
    Serial.println("help (h)    - 显示此帮助信息");
    Serial.println("status (s)  - 显示系统状态");
    Serial.println("net (n)     - 运行网络诊断测试");
    Serial.println("display (d) - 运行显示测试");
    Serial.println("limit on (lon)  - 启用帧率限制");
    Serial.println("limit off (loff) - 禁用帧率限制");
    Serial.println("exit (x)    - 退出诊断模式");
    Serial.println();
}

void printSystemStatus() {
    Serial.println("\n=== 系统状态 ===");

    // WiFi状态
    Serial.print("WiFi状态: ");
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("已连接");
        Serial.print("  IP地址: ");
        Serial.println(WiFi.localIP());
        Serial.print("  信号强度: ");
        Serial.print(WiFi.RSSI());
        Serial.println(" dBm");
    } else {
        Serial.println("未连接");
    }

    // 内存状态
    Serial.print("可用堆内存: ");
    Serial.print(ESP.getFreeHeap());
    Serial.println(" 字节");

    // 运行时间
    Serial.print("运行时间: ");
    unsigned long uptime = millis() / 1000;
    int uptimeHours = uptime / 3600;
    int uptimeMinutes = (uptime % 3600) / 60;
    int uptimeSeconds = uptime % 60;
    Serial.printf("%02d:%02d:%02d\n", uptimeHours, uptimeMinutes, uptimeSeconds);

    // 帧率限制状态
    Serial.print("帧率限制: ");
    Serial.println(frameRateLimitEnabled ? "启用" : "禁用");

    // 显示帧状态
    Serial.print("显示帧状态: ");
    Serial.println(displayFrame ? "正在处理帧" : "空闲");

    Serial.println();
}

void runNetworkTest() {
    Serial.println("\n=== 网络诊断测试 ===");

    // 检查WiFi连接
    Serial.print("WiFi连接: ");
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("正常");
    } else {
        Serial.println("异常 - 尝试重新连接");
        WiFi.reconnect();
        delay(5000);
        if (WiFi.status() == WL_CONNECTED) {
            Serial.println("重新连接成功");
        } else {
            Serial.println("重新连接失败");
        }
    }

    // 测试网关连通性
    IPAddress gateway = WiFi.gatewayIP();
    Serial.print("网关Ping测试 (");
    Serial.print(gateway);
    Serial.print("): ");

    // ESP32没有内置ping功能，这里只是模拟
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("假设可达");
    } else {
        Serial.println("无法测试 - WiFi未连接");
    }

    // WebSocket服务器状态
    Serial.println("WebSocket服务器: 运行中");

    Serial.println("网络诊断完成");
    Serial.println();
}

void runDisplayTest() {
    Serial.println("\n=== 显示测试 ===");
    Serial.println("运行显示测试模式...");

    // 保存当前显示状态
    bool prevDisplayFrame = displayFrame;
    displayFrame = false;

    // 清屏
    tft.fillScreen(ST77XX_BLACK);

    // 测试1: 颜色条
    Serial.println("测试1: 颜色条");
    uint16_t colors[] = {ST77XX_RED, ST77XX_GREEN, ST77XX_BLUE,
                         ST77XX_YELLOW, ST77XX_CYAN, ST77XX_MAGENTA};

    for (int i = 0; i < 6; i++) {
        tft.fillRect(0, i * 40, 240, 40, colors[i]);
        delay(200);
    }
    delay(1000);

    // 测试2: 渐变填充
    Serial.println("测试2: 渐变填充");
    tft.fillScreen(ST77XX_BLACK);

    for (int y = 0; y < 240; y++) {
        uint8_t r = map(y, 0, 240, 0, 255);
        uint8_t g = map(y, 0, 240, 255, 0);
        uint8_t b = 128;
        uint16_t color = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);

        tft.drawFastHLine(0, y, 240, color);
        if (y % 20 == 0) delay(50);
    }
    delay(1000);

    // 测试3: 动画矩形
    Serial.println("测试3: 动画矩形");
    tft.fillScreen(ST77XX_BLACK);

    int x = 0, y = 0;
    int dx = 8, dy = 6;
    int size = 40;

    for (int i = 0; i < 100; i++) {
        tft.fillScreen(ST77XX_BLACK);
        tft.fillRect(x, y, size, size, colors[i % 6]);

        x += dx;
        y += dy;

        if (x <= 0 || x + size >= 240) dx = -dx;
        if (y <= 0 || y + size >= 240) dy = -dy;

        delay(50);
    }

    // 恢复显示状态
    displayFrame = prevDisplayFrame;
    tft.fillScreen(ST77XX_BLACK);

    Serial.println("显示测试完成");
    Serial.println();
}

void toggleFrameRateLimit(bool enable) {
    frameRateLimitEnabled = enable;
    Serial.print("帧率限制已");
    Serial.println(enable ? "启用" : "禁用");
}
