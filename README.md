# LearnWithBoards - ESP32 & 多开发板初学者项目合集

欢迎使用 **LearnWithBoards**！本仓库包含多个基于 PlatformIO 的嵌入式开发项目，支持多种开发板（如 ESP32、ESP32-C3、ESP8266、Arduino Uno 等）。项目涵盖文件操作、Wi-Fi 控制 LED 等功能，旨在帮助新手快速掌握嵌入式开发技能。

## 项目特点

- **跨平台支持**：兼容多种开发板，只需简单配置即可适配。
- **简单易懂**：代码注释详细，适合零基础学习。
- **实用功能**：包括 SD 卡读写、LED 控制、网页服务器等。

## 支持的开发板

- **ESP32**（如 ESP32-DevKitC）
- **ESP32-C3**（如 airm2m_core_esp32c3）
- **ESP8266**（如 NodeMCU）
- **Arduino Uno / Nano**

## 开发环境

- **硬件**：
  - ESP32、ESP32-C3 或 ESP8266 开发板。
  - Arduino 系列开发板（可能需要额外模块）。
- **软件**：
  - [PlatformIO](https://platformio.org/)（推荐使用 VS Code 插件）。
  - Arduino 框架。
- **依赖库**：
  - `WiFi.h`（ESP32 Wi-Fi 支持）
  - `ESP8266WiFi.h`（ESP8266 Wi-Fi 支持）
  - `FS.h`（文件系统支持）

## 快速开始

### 1. 克隆仓库

```bash
git clone https://github.com/14790897/LearnWithBoards.git
cd LearnWithBoards
```

### 2. 配置项目

使用 platformio 打开你需要的项目文件夹

- **重要：配置 Wi-Fi 项目**  
  如果使用 Wi-Fi 相关项目，需要创建 `src/secrets.h` 文件，并填入以下内容：

  ```cpp
  // src/secrets.h
  #ifndef SECRETS_H
  #define SECRETS_H

  const char *ssid = "你的WiFi名称";     // 替换为你的 Wi-Fi 名称
  const char *password = "你的WiFi密码"; // 替换为你的 Wi-Fi 密码

  #endif
  ```

  **注意**：`secrets.h` 已被 `.gitignore` 忽略，不会提交到仓库，请务必手动创建并填写你的 Wi-Fi 信息。

### 3. 编译与上传

- 点击“Build”编译代码。
- 点击“Upload”将代码烧录到开发板。

### 4. 测试

- 打开串口监视器（波特率 115200），查看运行日志。
- 对于 Wi-Fi 项目，在浏览器中输入开发板的 IP 地址（串口会打印），即可控制 LED。

## 注意事项

- **敏感信息**：请勿将 Wi-Fi 密码等信息直接写在代码中，始终使用 `secrets.h`。
- **调试**：使用串口日志（`Serial.println`）排查问题。

## 贡献与反馈

- 如果你有改进建议或新项目想法，欢迎提交 Pull Request！
- 遇到问题？请在 Issues 中提问，我会尽力解答。

## 许可证

本项目采用 [MIT 许可证](LICENSE)，你可以自由使用和修改代码。
