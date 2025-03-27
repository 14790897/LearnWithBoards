#include <Arduino.h>
#include <ESP8266WiFi.h>
#include "secrets.h" // 包含敏感信息的头文件

// 设置固定 IP 地址
IPAddress local_IP(192, 168, 0, 130); // 替换为你想要的固定 IP 地址
IPAddress gateway(192, 168, 0, 1);    // 替换为你的网关地址（通常是路由器的 IP）
IPAddress subnet(255, 255, 255, 0);   // 子网掩码
IPAddress dns(192, 168, 0, 1);        // 替换为 DNS 服务器地址（通常与网关地址相同）

WiFiServer server(80); // 创建一个 HTTP 服务器，端口 80

void setup()
{
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH); // 初始化 LED 为关闭
  Serial.println("测试串口日志打印...");

  // 配置固定 IP 地址
  if (!WiFi.config(local_IP, gateway, subnet, dns))
  {
    Serial.println("固定 IP 地址配置失败！");
  }

  // 连接到 Wi-Fi
  WiFi.begin(ssid, password); // 使用 secrets.h 中的变量
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    Serial.println("连接 Wi-Fi...");
  }
  Serial.println("Wi-Fi 已连接");
  Serial.print("IP 地址: ");
  Serial.println(WiFi.localIP()); // 打印设备的固定 IP 地址

  server.begin(); // 启动服务器
}

void loop()
{
  WiFiClient client = server.accept(); // 检查是否有客户端连接
  if (client)
  {
    Serial.println("新客户端连接");
    String request = client.readStringUntil('\r'); // 读取客户端请求
    Serial.println(request);
    client.flush();

    // 检测请求中的 LED 控制命令
    if (request.indexOf("/LED=ON") != -1)
    {
      digitalWrite(LED_BUILTIN, LOW); // 打开 LED
    }
    else if (request.indexOf("/LED=OFF") != -1)
    {
      digitalWrite(LED_BUILTIN, HIGH); // 关闭 LED
    }

    // 返回 HTML 页面
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/html");
    client.println("");
    client.println("<!DOCTYPE HTML>");
    client.println("<html>");
    client.println("<head>");
    client.println("<title>ESP8266 LED Control</title>");
    client.println("<style>");
    client.println("button {");
    client.println("  display: inline-block;");
    client.println("  padding: 10px 20px;");
    client.println("  font-size: 16px;");
    client.println("  margin: 10px;");
    client.println("  cursor: pointer;");
    client.println("  background-color: #4CAF50;");
    client.println("  color: white;");
    client.println("  border: none;");
    client.println("  border-radius: 5px;");
    client.println("}");
    client.println("button:hover { background-color: #45a049; }");
    client.println("</style>");
    client.println("</head>");
    client.println("<body>");
    client.println("<h1>ESP8266 LED Control</h1>");
    client.println("<p>Click the buttons below to control the onboard LED:</p>");
    client.println("<a href=\"/LED=ON\"><button>Turn ON LED</button></a>");
    client.println("<a href=\"/LED=OFF\"><button>Turn OFF LED</button></a>");
    client.println("</body>");
    client.println("</html>");

    client.stop();
    Serial.println("客户端断开连接");
  }
}