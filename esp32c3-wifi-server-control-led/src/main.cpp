#include <Arduino.h>
#include <WiFi.h>
#include "secrets.h" // 包含新的头文件

// 设置固定 IP 地址
IPAddress local_IP(192, 168, 0, 130);
IPAddress gateway(192, 168, 0, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress dns(192, 168, 0, 1);

WiFiServer server(80);

void setup()
{
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  Serial.println("测试串口日志打印...");

  if (!WiFi.config(local_IP, gateway, subnet, dns))
  {
    Serial.println("固定 IP 地址配置失败！");
  }

  WiFi.begin(ssid, password); // 使用 secrets.h 中的变量
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    Serial.println("连接 Wi-Fi...");
  }
  Serial.println("Wi-Fi 已连接");
  Serial.print("IP 地址: ");
  Serial.println(WiFi.localIP());

  server.begin();
}

// loop 函数保持不变
void loop()
{
  WiFiClient client = server.available();
  if (client)
  {
    Serial.println("新客户端连接");
    String request = client.readStringUntil('\r');
    Serial.println(request);
    client.flush();

    if (request.indexOf("/LED=ON") != -1)
    {
      digitalWrite(LED_BUILTIN, LOW);
      Serial.println("打开 LED");
    }
    else if (request.indexOf("/LED=OFF") != -1)
    {
      digitalWrite(LED_BUILTIN, HIGH);
      Serial.println("关闭 LED");
    }

    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/html");
    client.println("");
    client.println("<!DOCTYPE HTML>");
    client.println("<html>");
    client.println("<head>");
    client.println("<title>ESP32-C3 LED Control</title>");
    client.println("<style>");
    client.println("button { display: inline-block; padding: 10px 20px; font-size: 16px; margin: 10px; cursor: pointer; background-color: #4CAF50; color: white; border: none; border-radius: 5px; }");
    client.println("button:hover { background-color: #45a049; }");
    client.println("</style>");
    client.println("</head>");
    client.println("<body>");
    client.println("<h1>ESP32-C3 LED Control</h1>");
    client.println("<p>Click the buttons below to control the onboard LED:</p>");
    client.println("<a href=\"/LED=ON\"><button>Turn ON LED</button></a>");
    client.println("<a href=\"/LED=OFF\"><button>Turn OFF LED</button></a>");
    client.println("</body>");
    client.println("</html>");

    client.stop();
    Serial.println("客户端断开连接");
  }
}