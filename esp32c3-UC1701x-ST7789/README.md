# ESP32 ST7789 Video Frame Display

这是一个基于 ESP32 和 ST7789 显示屏的项目，通过 WiFi 从网页前端接收视频帧数据并实时显示。项目使用 WebSocket 协议传输压缩后的二值帧数据，支持 240x240 分辨率的动画和视频播放。

## 演示视频

https://www.bilibili.com/video/BV1g2R2Y8Esm/

## 功能特性

- **硬件支持**：使用 ESP32-C3 和 ST7789 TFT 显示屏（240x240 分辨率）。
- **WiFi 连接**：静态 IP 配置，支持 WebSocket 服务器。
- **视频帧处理**：
  - 前端：上传视频文件，提取帧并进行二值化处理。
  - 数据压缩：将 240x240 像素的二值数据打包为 7200 字节（8 像素/字节）。
  - 传输：通过 WebSocket 实时发送到 ESP32。
- **显示模式**：
  - 帧显示模式：接收并显示视频帧。
- **高效渲染**：使用批量像素绘制优化显示性能。
- **诊断模式**：内置诊断功能，帮助排查连接和显示问题。
- **帧率控制**：可通过诊断模式动态调整帧率限制。

## 硬件要求

- **ESP32-C3 开发板**（如 AirM2M Core ESP32C3 或类似型号）。
- **ST7789 TFT 显示屏**（240x240 分辨率）。
- **引脚连接**：
  - CS: GPIO 7
  - DC: GPIO 6
  - RST: GPIO 10
  - SDA, SCL: SPI 接口
- **WiFi 网络**：需要连接到与前端相同的局域网。

## 软件依赖

- **PlatformIO**：用于构建和上传固件。
- **库依赖**（在 `platformio.ini` 中定义）：
  - `Adafruit GFX Library`
  - `Adafruit ST7789`
  - `WiFi`
  - `WebSockets`
  - `ArduinoJson`

## 安装步骤

### 1. 克隆仓库

```bash
git clone 我的仓库地址
cd 目标目录
```

### 2. 配置 WiFi 凭据

- 创建 `src/include/secrets.h` 文件，添加你的 WiFi SSID 和密码：

  ```cpp
  #ifndef SECRETS_H
  #define SECRETS_H
  const char* ssid = "your_wifi_ssid";
  const char* password = "your_wifi_password";
  #endif
  ```

### 3. 配置 PlatformIO

- 编辑 `platformio.ini`：

  ```ini
  [platformio]
  default_envs = esp32_serial

  [common]
  platform = espressif32
  board = esp32-c3-devkitm-1  ; 替换为你的板子型号，例如 airm2m_core_esp32c3
  framework = arduino
  monitor_speed = 115200
  upload_speed = 921600
  lib_deps =
      Adafruit GFX Library
      Adafruit ST7789
      WiFi
      WebSockets
      ArduinoJson

  [env:esp32_serial]
  extends = common
  upload_protocol = esptool
  upload_port = COM5  ; 替换为你的串口，例如 /dev/ttyUSB0

  [env:esp32_ota]
  extends = common
  upload_protocol = espota
  upload_port = 192.168.0.10
  ```

- 根据你的硬件调整 `board` 和 `upload_port`。

### 4. 编译和上传固件

- 通过串口上传：

  ```bash
  pio run -e esp32_serial -t upload
  ```

- 或通过 OTA 上传（首次需串口上传）：

  ```bash
  pio run -e esp32_ota -t upload
  ```

### 5. 运行前端

- 将 `frontend/index.html` 托管到本地服务器，或直接在浏览器打开（需支持 WebSocket）。
- 确保前端的 `ESP32_IP`（默认为 `ws://192.168.0.10:80`）与 ESP32 的实际 IP 匹配。
- 也可以使用 `frontend/connection_test.html` 测试 WebSocket 连接。

## 使用方法

### 1. 启动设备

- 上电后，ESP32 连接 WiFi 并显示 IP 地址（默认 `192.168.0.10`）。
- 默认情况下，屏幕显示 IP 地址。

### 2. 上传视频

- 打开 `index.html`，选择一个视频文件（`.mp4`、`.webm` 等格式）。
- 点击“处理并发送”按钮：
  - 视频将以 5 帧/秒（可调整 `FRAME_INTERVAL`）提取帧。
  - 每帧转换为 240x240 的二值数据并通过 WebSocket 发送。

### 3. 显示视频帧

- ESP32 接收到帧数据后，屏幕显示二值化的视频帧。
- 界面上显示连接状态、队列状态和处理进度。

## 示例帧数据

- **原始帧**：240x240 像素的 RGB 图像。
- **二值化**：灰度值 > 128 转为 1（白色），否则为 0（黑色）。
- **压缩**：57600 位打包为 7200 字节。
- **传输**：通过 WebSocket 发送二进制数据。

## 注意事项

- **内存**：ESP32 SRAM 需支持 57KB 的 `frameBuffer`，避免定义过多大数组。
- **网络**：确保 ESP32 和前端在同一局域网，防火墙不阻挡 WebSocket（端口 80）。
- **性能**：
  - 前端处理 240x240 帧可能较慢，建议测试短视频。
  - ESP32 显示大帧时可能有延迟，可降低帧率（调整 `FRAME_INTERVAL`）。
- **调试**：
  - 通过串口监视器（115200 波特率）查看 WiFi 和 WebSocket 状态。
  - 使用诊断模式排查问题（输入 `diag` 进入）。

## 项目结构

```text
esp32-st7789-video-display/
├── src/
│   ├── main.cpp          ; ESP32 主程序
│   ├── ST7789.cpp        ; ST7789 显示驱动和WebSocket处理
│   ├── ST7789.h          ; ST7789 头文件
│   ├── DiagnosticMode.cpp ; 诊断模式实现
│   ├── DiagnosticMode.h   ; 诊断模式头文件
│   └── secrets.h         ; WiFi 凭据（需手动创建）
├── frontend/
│   ├── index.html        ; 主前端界面
│   └── connection_test.html ; 连接测试工具
├── platformio.ini        ; PlatformIO 配置文件
└── README.md             ; 本文档
```

## 诊断模式

项目内置了强大的诊断模式，帮助排查各种问题：

### 进入诊断模式

1. 打开串口监视器（波特率 115200）
2. 输入 `diag` 或 `diagnostic` 并发送
3. 系统将显示诊断模式帮助信息

### 可用诊断命令

- `help` 或 `h` - 显示帮助信息
- `status` 或 `s` - 显示系统状态（WiFi、内存、运行时间等）
- `net` 或 `n` - 运行网络诊断测试
- `display` 或 `d` - 运行显示测试
- `limit on` 或 `lon` - 启用帧率限制
- `limit off` 或 `loff` - 禁用帧率限制
- `exit` 或 `x` - 退出诊断模式

### 排查传输卡顿问题

如果遇到传输卡顿问题，可以尝试：

1. 进入诊断模式，使用 `status` 命令检查系统状态
2. 使用 `net` 命令检查网络连接
3. 尝试使用 `limit off` 命令关闭帧率限制
4. 在前端增加 `FRAME_INTERVAL` 值，减少帧率
5. 使用连接测试工具检查 WebSocket 连接稳定性

## 故障排除

### 传输卡顿问题

如果遇到传输卡顿：

1. 检查 WiFi 信号强度（使用诊断模式的 `status` 命令）
2. 增加前端的 `FRAME_INTERVAL` 值（默认 200ms）
3. 尝试关闭帧率限制（诊断模式中使用 `limit off`）
4. 检查 ESP32 的可用内存（诊断模式中使用 `status`）

### 连接问题

如果无法连接：

1. 确认 ESP32 的 IP 地址（启动时显示在屏幕上）
2. 确保前端的 WebSocket URL 正确（默认 `ws://192.168.0.10:80`）
3. 使用连接测试工具进行 Ping 测试
4. 检查 WiFi 网络设置和防火墙

## 贡献

欢迎提交 Issue 或 Pull Request，优化代码或添加新功能！

## 许可

[MIT License](LICENSE)
