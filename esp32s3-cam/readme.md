# ESP32-Vlog-Camera

## 1. 项目简介

这是一个基于ESP32-S3微控制器和OV5640摄像头传感器的多功能相机系统。该项目实现了视频录制、实时流媒体预览和陀螺仪数据采集等功能，可用于运动相机、监控系统或其他需要图像捕获的应用场景。

### 硬件组成

- **主控制器**：ESP32-S3（双核处理器，支持Wi-Fi和蓝牙连接）
- **摄像头**：OV5640（500万像素CMOS传感器）
- **存储**：MicroSD卡（用于视频录制）
- **运动传感器**：MPU6050（6轴陀螺仪和加速度计）
- **电源**：USB供电或锂电池

### 主要功能

#### 视频录制
- 支持高清视频录制（最高支持SVGA 800x600分辨率）
- 视频文件保存到MicroSD卡
- 录制过程中可同步采集陀螺仪数据（兼容Gyroflow稳定软件）
- 可调节帧率（1-60 FPS）

#### 实时预览
- 通过Wi-Fi网络提供实时相机画面预览
- 按需获取图像帧，减少资源消耗
- 支持在网页浏览器中查看

#### 图像控制
- 支持垂直翻转和水平镜像功能
- 可调节亮度和饱和度

#### 陀螺仪数据
- 可选择开启/关闭MPU6050陀螺仪数据采集
- 陀螺仪数据可用于后期视频稳定处理

#### 网页界面
- 直观的用户界面，支持所有功能控制
- 实时显示录制状态、文件大小和帧率
- 支持移动设备访问

### 技术特点

- **Wi-Fi接入点模式**：创建独立Wi-Fi网络，无需外部路由器
- **Web服务器**：内置HTTP服务器提供用户界面和API
- **高效编码**：优化的JPEG编码实现流畅的视频录制
- **低延迟**：针对实时预览优化的图像传输
- **可扩展**：模块化设计，易于添加新功能

### 应用场景

- 运动相机（如自行车、滑板等运动记录）
- 安全监控系统
- 机器人视觉
- DIY无人机
- 科学实验记录
- 教育演示

### 未来计划

- 添加H.264硬件编码支持，提高视频质量和减小文件大小
- 实现Wi-Fi直连模式，支持手机APP控制
- 添加运动检测和自动录制功能
- 优化电源管理，延长电池使用时间
- 支持更多摄像头传感器型号

## 2. 使用说明

### 硬件连接

待补充...

### 软件安装

待补充...

### 操作指南

待补充...

## 3. 开发文档

待补充...

## 4. 参考资料

- [ESP32-MPU6050参考](https://randomnerdtutorials.com/esp32-mpu-6050-accelerometer-gyroscope-arduino/)
- 视频转换命令: `ffmpeg -r 5 -i video.mjpeg -vf "vflip,hflip" -c:v libx264 output.mp4`

## 5. 许可证

本项目采用MIT许可证开源，欢迎贡献代码和提出改进建议。

<!-- git submodule add https://github.com/14790897/ESP32-Vlog-Camera.git ESP32-Vlog-Camera -->


<!-- - [ESP32-MPU6050参考](https://randomnerdtutorials.com/esp32-mpu-6050-accelerometer-gyroscope-arduino/)
- 视频转换命令: `ffmpeg -f mjpeg -r 5 -i video.mjpeg -vf "vflip,hflip" -c:v libx264 output.mp4` 
- 视频修复命令：`ffmpeg -f mjpeg -i video.mjpeg_20250510_093128.mjpeg -c copy recovered.avi
  ffmpeg -i recovered.avi -vf "vflip,hflip" -c:v libx264 -preset veryfast -crf 23 output.mp4`-->
<!-- 翻转参考:https://blog.csdn.net/Turn_on/article/details/105908360 -->