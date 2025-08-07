# Arduino UNO R3 TFT LCD 显示项目

这是一个基于 Arduino UNO R3 和 ST7735 TFT LCD 显示屏的项目，实现了文本分页显示和雪花动画效果。

## 项目概述

本项目包含三个主要功能模块：
- **文本分页显示** (`main.cpp`) - 支持长文本的自动分页和循环显示
- **雪花动画** (`snowflake.cpp`) - 实现动态雪花飘落效果
- **简单文本显示** (`text.cpp`) - 基础文本显示功能

## 硬件要求

### 主要组件
- Arduino UNO R3 开发板
- ST7735 TFT LCD 显示屏 (128x160 像素)
- 杜邦线若干

### 接线说明
| ST7735 引脚 | Arduino UNO 引脚 | 说明 |
|-------------|------------------|------|
| VCC | 5V | 电源正极 |
| GND | GND | 电源负极 |
| SCL/SCLK | 引脚 13 (SCK) | SPI 时钟线 |
| SDA/MOSI | 引脚 11 (MOSI) | SPI 数据线 |
| RES/RESET | 引脚 8 | 复位引脚 |
| DC/A0 | 引脚 9 | 数据/命令选择 |
| CS | 引脚 10 | 片选引脚 |

## 软件环境

### 开发环境
- **PlatformIO** - 推荐的开发环境
- **Arduino IDE** - 也可使用传统 Arduino IDE

### 依赖库
项目使用以下库（已在 `platformio.ini` 中配置）：
- `SPI` - Arduino 内置 SPI 库
- `Adafruit ST7735 and ST7789 Library` (v1.11.0)
- `Adafruit GFX Library` (v1.11.9)
- `Ucglib` (v1.3.3) - 主要使用的图形库
- `U8g2` - 图形显示库

## 功能特性

### 1. 文本分页显示 (`main.cpp`)
- **自动分页**：长文本自动按屏幕尺寸分页
- **循环显示**：页面自动循环切换（每5秒）
- **看门狗保护**：防止程序死锁
- **串口调试**：输出分页信息到串口

**主要参数：**
- 屏幕尺寸：130x90 像素（显示区域）
- 字符宽度：10 像素
- 行高：16 像素
- 最大行数：5 行/页

### 2. 雪花动画 (`snowflake.cpp`)
- **动态雪花**：最多100个雪花同时显示
- **随机生成**：雪花位置和大小随机
- **流畅动画**：5ms 刷新间隔
- **循环效果**：雪花落到底部后重新生成

**动画参数：**
- 最大雪花数：100
- 雪花大小：2-4 像素
- 下落速度：1 像素/帧
- 激活概率：30%

### 3. 简单文本显示 (`text.cpp`)
- **基础显示**：简单文本显示功能
- **分页支持**：支持文本分页
- **定时切换**：5秒自动切换页面

## 使用方法

### 1. 环境准备
```bash
# 安装 PlatformIO
pip install platformio

# 或使用 VS Code 扩展
# 在 VS Code 中安装 PlatformIO IDE 扩展
```

### 2. 编译和上传
```bash
# 进入项目目录
cd unor3-tft-lcd

# 编译项目
pio run

# 上传到开发板
pio run --target upload

# 打开串口监视器
pio device monitor
```

### 3. 切换不同功能
项目包含三个不同的 `.cpp` 文件，要使用不同功能需要：

1. 将对应的 `.cpp` 文件重命名为 `main.cpp`
2. 将原来的 `main.cpp` 重命名为其他名称
3. 重新编译上传

例如，使用雪花动画：
```bash
mv src/main.cpp main_text.cpp
mv snowflake.cpp src/main.cpp
pio run --target upload
```

## 配置说明

### 显示屏配置
```cpp
// 显示屏初始化（硬件SPI）
Ucglib_ST7735_18x128x160_HWSPI ucg(9, 10, 8);
// 参数：DC引脚, CS引脚, RESET引脚
```

### 文本显示参数
```cpp
#define SCREEN_WIDTH 130        // 显示宽度
#define CHAR_WIDTH 10          // 字符宽度
#define SCREEN_HEIGHT 90       // 显示高度
#define LINE_HEIGHT 16         // 行高
#define MAX_LINES 5           // 每页最大行数
```

### 动画参数
```cpp
#define MAX_SNOWFLAKES 100     // 最大雪花数
const unsigned long updateInterval = 5000;  // 页面切换间隔(ms)
```

## 故障排除

### 常见问题

1. **显示屏无显示**
   - 检查接线是否正确
   - 确认电源供电正常
   - 检查引脚定义是否匹配

2. **编译错误**
   - 确保所有依赖库已安装
   - 检查 `platformio.ini` 配置
   - 更新库到最新版本

3. **串口无输出**
   - 检查波特率设置（115200）
   - 确认 USB 连接正常
   - 检查串口驱动

4. **程序运行异常**
   - 看门狗可能触发重启
   - 检查内存使用情况
   - 增加延时或优化代码

### 调试技巧
- 使用串口监视器查看调试信息
- 检查 `Serial.println()` 输出
- 使用 LED 指示程序状态
- 逐步注释代码定位问题

## 扩展功能

### 可能的改进方向
- 添加按钮控制页面切换
- 支持更多字体和字符集
- 实现触摸屏交互
- 添加温度传感器显示
- 支持图片显示
- 实现菜单系统

### 硬件扩展
- 添加 SD 卡模块存储更多内容
- 集成 RTC 模块显示时间
- 添加蜂鸣器音效
- 连接传感器显示数据

## 许可证

本项目采用 MIT 许可证，详见 LICENSE 文件。

## 贡献

欢迎提交 Issue 和 Pull Request 来改进这个项目。

## 联系方式

如有问题或建议，请通过以下方式联系：
- 提交 GitHub Issue
- 发送邮件至项目维护者

---

**注意**：使用前请确保正确连接硬件，避免短路或接错线导致设备损坏。
