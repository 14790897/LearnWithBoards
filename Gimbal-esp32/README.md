# Gimbal-esp32 超声波自动跟踪舵机

本项目基于 ESP32、舵机和超声波传感器，实现自动跟踪距离最近物体的功能。

## 硬件需求
- ESP32 开发板
- 舵机（如 SG90）
- 超声波测距模块（如 HC-SR04）
- 杜邦线若干

## 默认引脚定义
> 注意：ESP32 某些引脚有特殊用途，建议根据实际板型调整
- TRIG_PIN: 3
- ECHO_PIN: 10
- SERVO_PIN: 9

如遇测距异常，请将引脚更换为 12、13、14、15、16、17、18、19、21、22、23、25、26、27 等推荐 GPIO。

## 功能说明
- 舵机每次只检测左右两个方向的距离（不检测正前方）。
- 只跟踪距离 20cm 以内的物体。
- 若本次未检测到目标，则不更新距离，保持上次状态。
- 串口输出当前角度和左右距离，便于调试。

## 使用方法
1. 按照代码中的引脚定义连接硬件。
2. 使用 PlatformIO 或 Arduino IDE 上传 `src/main.cpp` 到 ESP32。
3. 打开串口监视器（115200 波特率），观察调试信息。
4. 调整目标物体位置，舵机会自动转向距离最近的一侧。

## 代码结构
- `main.cpp`：主程序，包含测距、舵机控制和跟踪逻辑。

## 常见问题
- 串口输出距离均为 9999：请检查超声波模块接线、电源和引脚分配。
- 舵机抖动或无动作：检查供电和舵机 PWM 引脚。
- duration 一直为 0：更换引脚，确保目标物体在有效测距范围内。

## 许可
MIT License
