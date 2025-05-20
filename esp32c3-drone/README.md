# ESP32-C3 无人机控制项目

这是一个基于 ESP32-C3 的无人机控制项目，包含电机控制和 LED 控制功能，并提供了一个 Web 界面进行远程控制。

## 功能特点

- 四个电机的 PWM 控制
- LED 控制（开关和亮度调节）
- Web 界面远程控制
- OTA（空中升级）支持
- 静态 IP 配置

## 硬件连接

- 电机 1: IA - GPIO11, IB - GPIO7
- 电机 2: IA - GPIO4, IB - GPIO5
- 电机 3: IA - GPIO3, IB - GPIO2
- 电机 4: IA - GPIO6, IB - GPIO10
- LED: GPIO8

## 软件依赖

- ESPAsyncWebServer
- AsyncTCP
- ArduinoJson
- LittleFS 文件系统

## 使用方法

1. 将代码上传到 ESP32-C3 开发板
2. 上传 LittleFS 文件系统（包含 Web 界面文件）
3. 设备将连接到配置的 WiFi 网络
4. 通过浏览器访问设备的 IP 地址（默认为 192.168.0.10）

## Web 界面功能

- LED 控制（开关和亮度调节）
- 设备信息显示
- 设置配置（主机名、WiFi 等）

## 开发环境

- PlatformIO
- Arduino 框架
- ESP32-C3 开发板

## 注意事项

- 确保 WiFi 凭据在 `secrets.h` 文件中正确配置
- 默认静态 IP 为 192.168.0.10，可以在代码中修改
- OTA 更新默认启用，无需密码
