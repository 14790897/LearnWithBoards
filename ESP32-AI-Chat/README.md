# ESP32 AI Chat

一个基于ESP32的AI聊天程序，可以通过Web界面或串口与OpenAI API进行对话。该项目提供了一个轻量级但功能强大的界面，让您可以在ESP32设备上与AI助手进行交互。

## 功能特点

- **多种交互方式**：
  - 支持通过Web界面与AI聊天
  - 支持通过串口与AI聊天
  - 响应式设计，适配桌面和移动设备

- **网络功能**：
  - 使用mDNS实现友好的主机名访问
  - 支持WiFi站点模式和AP模式
  - 安全的HTTPS API调用

- **存储功能**：
  - 使用LittleFS文件系统存储Web界面文件
  - 使用NVS（非易失性存储）保存对话历史
  - 支持清除历史记录

- **AI对话增强**：
  - 发送最近三轮对话历史给API以提供上下文
  - 支持连续对话，AI能够理解上下文

- **用户界面**：
  - 现代化的Web界面设计
  - 支持深色/浅色模式自动切换
  - 全屏显示支持
  - 动画效果和打字指示器

## 硬件要求

- ESP32开发板
- USB数据线

## 软件依赖

- Arduino框架
- ArduinoJson库
- LittleFS_esp32库

## 安装步骤

1. 克隆或下载本仓库
2. 使用PlatformIO打开项目
3. 创建`include/secrets.h`文件并配置以下参数：

```cpp
// WiFi设置
#define WIFI_SSID "YourWiFiSSID"      // 您的WiFi网络名称
#define WIFI_PASSWORD "YourWiFiPassword"  // 您的WiFi密码

// AP模式设置
#define AP_SSID "ESP32-AI-Chat"     // 热点名称
#define AP_PASSWORD "chatpassword"  // 热点密码（至少8个字符）

// 模式选择：true=AP模式，false=站点模式
#define USE_AP_MODE false

// Web服务器端口
#define WEB_SERVER_PORT 80

// mDNS主机名
#define MDNS_HOST_NAME "esp32-ai-chat"

// OpenAI API设置
#define API_KEY "your-openai-api-key"  // 您的OpenAI API密钥
#define API_SERVER "api.openai.com"    // API服务器
#define API_PORT 443                   // API端口
#define API_MODEL "gpt-3.5-turbo"      // 使用的模型
```

4. 编译并上传代码到ESP32：
   - 使用快捷键：`Ctrl+Alt+U`
   - 或使用命令行：
     ```bash
     pio run --target upload
     ```

5. 上传文件系统镜像到ESP32：
   - 使用PlatformIO侧边栏中的"Upload Filesystem Image"任务
   - 或使用命令行：
     ```bash
     pio run --target uploadfs
     ```

## 上传文件系统镜像

上传文件系统镜像是必要的步骤，否则Web界面将无法正常工作。有以下几种方法可以上传文件系统镜像：

### 方法1：使用快捷键

在PlatformIO环境中，使用快捷键 `Ctrl+Alt+S` 可以直接上传文件系统镜像到ESP32设备。

### 方法2：使用PlatformIO界面

1. 在PlatformIO侧边栏中，展开您的项目
2. 点击"Platform"
3. 找到并点击"Upload Filesystem Image"任务

### 方法3：使用命令行

```bash
pio run --target uploadfs
```

### 方法4：使用自定义任务

1. 在PlatformIO侧边栏中，展开您的项目
2. 点击"Custom"
3. 找到并点击"Upload LittleFS Image"任务

## 使用方法

### Web界面访问

1. 连接到ESP32的WiFi网络（AP模式）或将ESP32连接到您的WiFi网络（站点模式）
2. 在浏览器中访问：
   - `http://esp32-ai-chat.local`（使用mDNS）
   - 或ESP32的IP地址
3. 在聊天界面中输入消息并发送
4. 查看AI的回复
5. 您的对话历史将自动保存，并在下次访问时显示

### 串口访问

1. 打开串口监视器（波特率115200）
2. 输入消息并按回车键发送
3. 查看AI的回复

### 全屏模式

在移动设备上，点击界面任意位置将尝试进入全屏模式。如果自动全屏失败，屏幕右下角将显示全屏按钮。

### 清除历史记录

点击界面左上角的垃圾桶图标可以清除所有对话历史。

## 配置选项

所有配置选项都在`include/secrets.h`文件中，主要包括：

- **WiFi设置**：网络名称和密码
- **AP模式设置**：热点名称和密码
- **模式选择**：站点模式或AP模式
- **Web服务器设置**：端口号
- **mDNS设置**：主机名
- **OpenAI API设置**：API密钥、服务器、端口和模型

## 高级功能

### 对话历史上下文

系统会自动将最近三轮对话发送给OpenAI API，使得AI能够理解对话的上下文。这使得连续对话更加自然和连贯。

### NVS存储

对话历史使用ESP32的NVS（非易失性存储）进行保存，即使设备重启或断电，历史记录也不会丢失。

### 响应式设计

Web界面采用响应式设计，可以自动适应不同尺寸的屏幕，包括手机、平板和桌面设备。

### 深色模式

Web界面会根据系统设置自动切换深色/浅色模式，提供更好的视觉体验。

## 故障排除

### 无法连接到WiFi

- 检查`secrets.h`文件中的WiFi凭证是否正确
- 确保ESP32在WiFi覆盖范围内
- 尝试切换到AP模式（在`secrets.h`中设置`USE_AP_MODE`为`true`）

### Web界面无法加载

- 确保已经上传了文件系统镜像
- 检查串口监视器中的错误消息
- 尝试使用IP地址而不是mDNS名称访问

### API调用失败

- 检查`secrets.h`文件中的API密钥是否正确
- 确保ESP32可以访问互联网
- 检查串口监视器中的错误消息

### 历史记录不显示

- 尝试清除浏览器缓存并重新加载页面
- 检查串口监视器中的调试输出
- 如果问题仍然存在，可以尝试清除历史记录

### API调用超时

- 如果遇到"ERROR: Client Timeout waiting for OpenAI response!"错误，请检查网络连接
- 确保您的WiFi信号强度足够
- 如果问题持续存在，可以尝试减少发送的历史记录数量

## 贡献

欢迎提交问题报告和功能请求。如果您想贡献代码，请提交Pull Request。

## 许可证

MIT

## 免责声明

本项目仅供学习和个人使用。使用OpenAI API时，请遵守OpenAI的使用条款和服务条款。

## 文件结构

- `src/main.cpp` - 主程序文件
- `include/secrets.h` - 配置文件
- `/data/index.html` - 主Web界面
- `/data/fallback.html` - 备用Web界面（当主界面不可用时使用）


## 参考文章
extra_scripts使用方法: https://dokk.org/documentation/platformio/v3.6.4/projectconf/advanced_scripting/