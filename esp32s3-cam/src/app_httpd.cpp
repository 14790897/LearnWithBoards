// Copyright 2015-2016 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#include "app_httpd.h"
#include "esp_http_server.h"
#include "esp_timer.h"
#include "esp_camera.h"
#include "esp32-hal-ledc.h"
#include "sdkconfig.h"
#include "SD_MMC.h"
#include "FS.h"
#include "esp_system.h"
#include <string>
#include "time.h" // Include time library for timestamp
#include "mpu_handler.h" // Add MPU handler include
#include "esp_sntp.h" // Add for NTP time sync
#if defined(ARDUINO_ARCH_ESP32) && defined(CONFIG_ARDUHAL_ESP_LOG)
#include "esp32-hal-log.h"
#endif
#define SD_MMC_CMD 38 // Please do not modify it.
#define SD_MMC_CLK 39 // Please do not modify it.
#define SD_MMC_D0 40  // Please do not modify it.

// Enable LED FLASH setting
#define CONFIG_LED_ILLUMINATOR_ENABLED 1

// LED FLASH setup
#if CONFIG_LED_ILLUMINATOR_ENABLED

#define LED_LEDC_GPIO 2 // configure LED pin
#define CONFIG_LED_MAX_INTENSITY 255

int led_duty = 0;
bool isStreaming = false;

#endif

typedef struct
{
    httpd_req_t *req;
    size_t len;
} jpg_chunking_t;

#define PART_BOUNDARY "123456789000000000000987654321"
static const char *_STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char *_STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char *_STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\nX-Timestamp: %d.%06d\r\n\r\n";

httpd_handle_t stream_httpd = NULL;
httpd_handle_t camera_httpd = NULL;

typedef struct
{
    size_t size;  // number of values used for filtering
    size_t index; // current value index
    size_t count; // value count
    int sum;
    int *values; // array to be filled with values
} ra_filter_t;

static ra_filter_t ra_filter;

// Define recording variables
bool isRecording = false;
File videoFile;
const char *videoFilePath = "/video.mjpeg";
unsigned long recordingStartTime = 0;
String currentRecordingPath = "";
size_t currentRecordingSize = 0;
int currentFrameCount = 0;
float currentFps = 0.0;

static TaskHandle_t recordTaskHandle = NULL;

// Declare external variable for MPU control
extern bool enableMPU;

// FPS control variables
int targetFPS = 10;                    // Default target FPS
int minFrameTimeMs = 1000 / targetFPS; // Minimum time between frames in milliseconds

// HTML page with recording controls
static const char RECORDING_CONTROL_HTML[] = R"rawliteral(
<!DOCTYPE html>
<html lang="zh-CN">
<head>
    <title>ESP32-S3-EYE 运动相机控制</title>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <style>
        :root {
            --primary-color: #3498db;
            --secondary-color: #2ecc71;
            --danger-color: #e74c3c;
            --dark-color: #2c3e50;
            --light-color: #ecf0f1;
            --border-radius: 8px;
            --box-shadow: 0 4px 6px rgba(0, 0, 0, 0.1);
        }
        * {
            box-sizing: border-box;
            margin: 0;
            padding: 0;
        }
        body {
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            line-height: 1.6;
            color: #333;
            background-color: #f9f9f9;
            padding: 20px;
            max-width: 1200px;
            margin: 0 auto;
        }
        header {
            text-align: center;
            margin-bottom: 20px;
        }
        h1 {
            color: var(--dark-color);
            font-size: 2rem;
            margin-bottom: 10px;
        }
        .container {
            display: flex;
            flex-direction: column;
            gap: 20px;
        }
        .card {
            background: white;
            border-radius: var(--border-radius);
            box-shadow: var(--box-shadow);
            padding: 20px;
            transition: transform 0.3s ease;
        }
        .card:hover {
            transform: translateY(-5px);
        }
        .card-header {
            display: flex;
            justify-content: space-between;
            align-items: center;
            margin-bottom: 15px;
            padding-bottom: 10px;
            border-bottom: 1px solid #eee;
        }
        .card-title {
            font-size: 1.2rem;
            font-weight: bold;
            color: var(--dark-color);
        }
        .controls {
            display: flex;
            flex-wrap: wrap;
            gap: 10px;
            justify-content: center;
        }
        .btn {
            display: inline-block;
            padding: 12px 24px;
            font-size: 1rem;
            font-weight: bold;
            text-align: center;
            text-decoration: none;
            cursor: pointer;
            border: none;
            border-radius: var(--border-radius);
            transition: all 0.3s ease;
            box-shadow: var(--box-shadow);
            color: white;
        }
        .btn-primary {
            background-color: var(--primary-color);
        }
        .btn-primary:hover {
            background-color: #2980b9;
        }
        .btn-success {
            background-color: var(--secondary-color);
        }
        .btn-success:hover {
            background-color: #27ae60;
        }
        .btn-danger {
            background-color: var(--danger-color);
        }
        .btn-danger:hover {
            background-color: #c0392b;
        }
        .btn:active {
            transform: translateY(2px);
            box-shadow: 0 2px 3px rgba(0, 0, 0, 0.1);
        }
        .status {
            display: inline-block;
            padding: 8px 16px;
            border-radius: 20px;
            font-weight: bold;
            background-color: var(--light-color);
            margin-top: 10px;
        }
        .status-recording {
            background-color: #ffeaa7;
            color: #d35400;
        }
        .status-stopped {
            background-color: #dfe6e9;
            color: #636e72;
        }
        .settings-grid {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(250px, 1fr));
            gap: 20px;
        }
        .setting-item {
            display: flex;
            flex-direction: column;
            gap: 10px;
        }
        .setting-label {
            font-weight: bold;
            display: flex;
            justify-content: space-between;
        }
        .setting-value {
            color: var(--primary-color);
            font-weight: bold;
        }
        .slider {
            -webkit-appearance: none;
            width: 100%;
            height: 8px;
            border-radius: 5px;
            background: #d3d3d3;
            outline: none;
        }
        .slider::-webkit-slider-thumb {
            -webkit-appearance: none;
            appearance: none;
            width: 20px;
            height: 20px;
            border-radius: 50%;
            background: var(--primary-color);
            cursor: pointer;
        }
        .slider::-moz-range-thumb {
            width: 20px;
            height: 20px;
            border-radius: 50%;
            background: var(--primary-color);
            cursor: pointer;
        }
        .toggle-switch {
            position: relative;
            display: inline-block;
            width: 60px;
            height: 34px;
        }
        .toggle-switch input {
            opacity: 0;
            width: 0;
            height: 0;
        }
        .toggle-slider {
            position: absolute;
            cursor: pointer;
            top: 0;
            left: 0;
            right: 0;
            bottom: 0;
            background-color: #ccc;
            transition: .4s;
            border-radius: 34px;
        }
        .toggle-slider:before {
            position: absolute;
            content: "";
            height: 26px;
            width: 26px;
            left: 4px;
            bottom: 4px;
            background-color: white;
            transition: .4s;
            border-radius: 50%;
        }
        input:checked + .toggle-slider {
            background-color: var(--secondary-color);
        }
        input:checked + .toggle-slider:before {
            transform: translateX(26px);
        }
        .notification {
            position: fixed;
            top: 20px;
            right: 20px;
            padding: 15px 20px;
            background-color: var(--dark-color);
            color: white;
            border-radius: var(--border-radius);
            box-shadow: var(--box-shadow);
            opacity: 0;
            transform: translateY(-20px);
            transition: all 0.3s ease;
            z-index: 1000;
        }
        .notification.show {
            opacity: 1;
            transform: translateY(0);
        }
        .stream-container {
            position: relative;
            width: 100%;
            border-radius: var(--border-radius);
            overflow: hidden;
            box-shadow: var(--box-shadow);
        }
        .stream-container img, .stream-container canvas {
            width: 100%;
            height: auto;
            display: block;
        }
        .stream-overlay {
            position: absolute;
            bottom: 10px;
            right: 10px;
            background-color: rgba(0, 0, 0, 0.5);
            color: white;
            padding: 5px 10px;
            border-radius: 4px;
            font-size: 0.9rem;
        }
        .stream-status {
            position: absolute;
            top: 50%;
            left: 50%;
            transform: translate(-50%, -50%);
            background-color: rgba(0, 0, 0, 0.7);
            color: white;
            padding: 10px 20px;
            border-radius: 4px;
            font-size: 1rem;
            text-align: center;
            max-width: 80%;
            display: none;
        }
        .stats {
            display: flex;
            flex-wrap: wrap;
            gap: 10px;
            margin-top: 10px;
        }
        .stat-item {
            background-color: var(--light-color);
            padding: 8px 12px;
            border-radius: var(--border-radius);
            font-size: 0.9rem;
        }
        .recording-stats {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));
            gap: 10px;
        }
        .stat-row {
            display: flex;
            justify-content: space-between;
            padding: 8px 12px;
            background-color: var(--light-color);
            border-radius: var(--border-radius);
            font-size: 0.9rem;
        }
        .stat-label {
            font-weight: bold;
            color: var(--dark-color);
        }
        .stat-value {
            color: var(--primary-color);
            font-weight: bold;
        }
        @media (max-width: 768px) {
            .container {
                gap: 15px;
            }
            .card {
                padding: 15px;
            }
            .btn {
                padding: 10px 20px;
                font-size: 0.9rem;
            }
            h1 {
                font-size: 1.5rem;
            }
        }
    </style>
</head>
<body>
    <header>
        <h1>ESP32-S3 运动相机</h1>
    </header>

    <div class="container">
        <!-- 视频流卡片 -->
        <div class="card">
            <div class="card-header">
                <div class="card-title">实时预览</div>
                <div id="status" class="status status-stopped">准备就绪</div>
            </div>
            <div class="stream-container">
                <canvas id="stream-canvas" width="640" height="480" style="width:100%;"></canvas>
                <div class="stream-overlay" id="fps-display">10 FPS</div>
                <div id="stream-status" class="stream-status"></div>
            </div>
            <div class="stats" id="stats">
                <div class="stat-item">分辨率: <span id="resolution">HD</span></div>
                <div class="stat-item">质量: <span id="quality">高</span></div>
            </div>
        </div>

        <!-- 录制控制卡片 -->
        <div class="card">
            <div class="card-header">
                <div class="card-title">录制控制</div>
            </div>
            <div class="controls">
                <button id="startBtn" class="btn btn-success">开始录制</button>
                <button id="stopBtn" class="btn btn-danger">停止录制</button>
            </div>

            <!-- 录制信息显示区 -->
            <div id="recording-info" class="recording-info" style="display: none; margin-top: 20px; padding-top: 15px; border-top: 1px solid #eee;">
                <h3 style="font-size: 1.1rem; margin-bottom: 10px;">当前录制信息</h3>
                <div class="recording-stats">
                    <div class="stat-row">
                        <span class="stat-label">文件名：</span>
                        <span id="rec-filename" class="stat-value">-</span>
                    </div>
                    <div class="stat-row">
                        <span class="stat-label">录制时长：</span>
                        <span id="rec-duration" class="stat-value">00:00:00</span>
                    </div>
                    <div class="stat-row">
                        <span class="stat-label">文件大小：</span>
                        <span id="rec-size" class="stat-value">0 KB</span>
                    </div>
                    <div class="stat-row">
                        <span class="stat-label">帧数：</span>
                        <span id="rec-frames" class="stat-value">0</span>
                    </div>
                    <div class="stat-row">
                        <span class="stat-label">实时帧率：</span>
                        <span id="rec-fps" class="stat-value">0 FPS</span>
                    </div>
                </div>
            </div>
        </div>

        <!-- 设置卡片 -->
        <div class="card">
            <div class="card-header">
                <div class="card-title">相机设置</div>
            </div>
            <div class="settings-grid">
                <div class="setting-item">
                    <div class="setting-label">
                        <span>帧率 (FPS)</span>
                        <span class="setting-value" id="fps-value">10</span>
                    </div>
                    <input type="range" min="5" max="60" value="10" class="slider" id="fps-slider">
                </div>

                <div class="setting-item">
                    <div class="setting-label">
                        <span>陀螺仪数据</span>                //     </div>
                    <label class="toggle-switch">
                        <input type="checkbox" id="mpu-toggle">
                        <span class="toggle-slider"></span>
                    </label>
                       </div>
                    </div>

        </div>
    </div>

    <div id="notification" class="notification"></div>

    <script>
        // DOM元素
        const statusEl = document.getElementById('status');
        const startBtn = document.getElementById('startBtn');
        const stopBtn = document.getElementById('stopBtn');
        const fpsSlider = document.getElementById('fps-slider');
        const fpsValue = document.getElementById('fps-value');
        const fpsDisplay = document.getElementById('fps-display');
        const mpuToggle = document.getElementById('mpu-toggle');
        const notificationEl = document.getElementById('notification');
        const streamCanvas = document.getElementById('stream-canvas');
        const streamStatus = document.getElementById('stream-status');

        // 录制信息元素
        const recordingInfoEl = document.getElementById('recording-info');
        const recFilenameEl = document.getElementById('rec-filename');
        const recDurationEl = document.getElementById('rec-duration');
        const recSizeEl = document.getElementById('rec-size');
        const recFramesEl = document.getElementById('rec-frames');
        const recFpsEl = document.getElementById('rec-fps');

        // 流媒体变量
        let streamActive = false;
        let mjpegStream = null;
        let lastFrameTime = 0;
        let frameCount = 0;
        let streamFps = 0;
        let streamInterval = null;

        // 初始化Canvas
        const ctx = streamCanvas.getContext('2d');

        // 设置Canvas尺寸为容器宽度
        function resizeCanvas() {
            const container = streamCanvas.parentElement;
            const containerWidth = container.clientWidth;
            // 保持原始宽高比 (4:3)
            const aspectRatio = 4/3;
            const canvasHeight = containerWidth / aspectRatio;

            // 设置画布尺寸
            streamCanvas.width = containerWidth;
            streamCanvas.height = canvasHeight;

            // 重绘初始内容
            drawInitialCanvas();
        }

        // 绘制初始内容
        function drawInitialCanvas() {
            ctx.fillStyle = '#f0f0f0';
            ctx.fillRect(0, 0, streamCanvas.width, streamCanvas.height);
            ctx.fillStyle = '#666';
            ctx.font = '20px Arial';
            ctx.textAlign = 'center';
            ctx.textBaseline = 'middle';
            ctx.fillText('点击开始流媒体', streamCanvas.width / 2, streamCanvas.height / 2);
        }

        // 初始调整尺寸
        resizeCanvas();

        // 当窗口大小变化时调整Canvas尺寸
        window.addEventListener('resize', resizeCanvas);

        // 显示流媒体状态
        function showStreamStatus(message, isError = false) {
            if (message) {
                streamStatus.textContent = message;
                streamStatus.style.display = 'block';
                streamStatus.style.backgroundColor = isError ? 'rgba(220, 53, 69, 0.7)' : 'rgba(0, 0, 0, 0.7)';
            } else {
                streamStatus.style.display = 'none';
            }
        }

        // 启动流媒体
        async function startStream() {
            // 检查是否正在录制
            if (statusEl.textContent === '正在录制') {
                showStreamStatus('无法在录制时播放流媒体', true);
                showNotification('无法在录制时流媒体播放', 5000);
                return;
            }

            // 如果已经有流媒体正在运行，先停止它
            if (streamActive) {
                await stopStream();
                // 等待一下，确保服务器有时间处理
                await new Promise(resolve => setTimeout(resolve, 500));
            }

            showStreamStatus('正在连接...');
            streamActive = true;

            // 计算帧间隔时间 (毫秒)
            const frameIntervalMs = 1000 / parseInt(fpsValue.textContent);
            console.log(`设置帧率: ${fpsValue.textContent} FPS (间隔: ${frameIntervalMs}ms)`);

            // 设置定时器来获取新的图像帧
            streamInterval = setInterval(fetchNewFrame, frameIntervalMs);

            // 计算FPS
            const fpsInterval = setInterval(calculateStreamFps, 1000);

            // 如果流媒体停止，清除FPS计算器
            const checkStreamInterval = setInterval(() => {
                if (!streamActive) {
                    clearInterval(fpsInterval);
                    clearInterval(checkStreamInterval);
                }
            }, 1000);

            // 立即获取第一帧
            await fetchNewFrame();
        }

        // 获取新的图像帧
        async function fetchNewFrame() {
            if (!streamActive) return;

            try {
                // 创建新的Image对象
                const img = new Image();

                // 设置加载事件
                img.onload = function() {
                    // 图像加载成功，隐藏状态消息
                    showStreamStatus(null);

                    // 更新当前图像
                    mjpegStream = img;

                    // 立即更新Canvas
                    updateCanvas();

                    // 增加帧计数
                    frameCount++;
                };

                img.onerror = function() {
                    // 如果是录制导致的错误，显示相应消息
                    if (statusEl.textContent === '正在录制') {
                        showStreamStatus('无法在录制时播放流媒体', true);
                        stopStream();
                    } else {
                        // 如果是临时错误，不停止流媒体，但显示错误状态
                        showStreamStatus('加载失败，正在重试...', true);
                    }
                };

                // 使用时间戳防止缓存
                img.src = '/stream?t=' + new Date().getTime();
            } catch (error) {
                console.error('Error fetching frame:', error);
                // 不停止流媒体，下一次定时器会再次尝试
            }
        }

        // 停止流媒体
        async function stopStream() {
            if (!streamActive) return;

            try {
                // 停止前端处理
                clearInterval(streamInterval);
                streamActive = false;

                // 重置Canvas
                drawInitialCanvas();

                // 清理资源
                if (mjpegStream) {
                    // 中断图像加载
                    mjpegStream.src = '';
                    mjpegStream = null;
                }

                // 清除状态显示
                showStreamStatus(null);

                showNotification('流媒体已停止');
            } catch (error) {
                console.error('Error stopping stream:', error);
                showNotification('停止流媒体时出错');
            }
        }

        // 更新Canvas
        function updateCanvas() {
            if (!streamActive || !mjpegStream) return;

            try {
                // 清除画布
                ctx.clearRect(0, 0, streamCanvas.width, streamCanvas.height);

                // 计算维持宽高比的绘制尺寸
                const imgWidth = mjpegStream.naturalWidth || 640;
                const imgHeight = mjpegStream.naturalHeight || 480;
                const imgAspect = imgWidth / imgHeight;
                const canvasAspect = streamCanvas.width / streamCanvas.height;

                let drawWidth, drawHeight, offsetX = 0, offsetY = 0;

                if (imgAspect > canvasAspect) {
                    // 图像更宽，适应高度
                    drawWidth = streamCanvas.width;
                    drawHeight = drawWidth / imgAspect;
                    offsetY = (streamCanvas.height - drawHeight) / 2;
                } else {
                    // 图像更高，适应宽度
                    drawHeight = streamCanvas.height;
                    drawWidth = drawHeight * imgAspect;
                    offsetX = (streamCanvas.width - drawWidth) / 2;
                }

                // 绘制图像
                ctx.drawImage(mjpegStream, offsetX, offsetY, drawWidth, drawHeight);
                frameCount++;
                lastFrameTime = performance.now();
            } catch (e) {
                console.error('Canvas更新错误:', e);
                console.error(e);
            }
        }

        // 计算流媒体FPS
        function calculateStreamFps() {
            if (!streamActive) {
                streamFps = 0;
                return;
            }

            streamFps = frameCount;
            frameCount = 0;
            fpsDisplay.textContent = `${streamFps} FPS`;
        }

        // 点击Canvas开始/停止流媒体
        streamCanvas.addEventListener('click', function() {
            if (streamActive) {
                stopStream();
            } else {
                startStream();
            }
        });

        // 显示通知
        function showNotification(message, duration = 3000) {
            notificationEl.textContent = message;
            notificationEl.classList.add('show');

            setTimeout(() => {
                notificationEl.classList.remove('show');
            }, duration);
        }

        // 格式化时间为 HH:MM:SS
        function formatTime(seconds) {
            const hrs = Math.floor(seconds / 3600);
            const mins = Math.floor((seconds % 3600) / 60);
            const secs = Math.floor(seconds % 60);
            return `${hrs.toString().padStart(2, '0')}:${mins.toString().padStart(2, '0')}:${secs.toString().padStart(2, '0')}`;
        }

        // 获取录制统计信息
        async function fetchRecordingStats() {
            try {
                const response = await fetch('/recording_stats');
                const data = await response.json();

                if (data.recording) {
                    // 显示录制信息区域
                    recordingInfoEl.style.display = 'block';

                    // 更新各项统计信息
                    recFilenameEl.textContent = data.filename || '-';
                    recDurationEl.textContent = formatTime(data.duration);
                    recSizeEl.textContent = data.size;
                    recFramesEl.textContent = data.frames;
                    recFpsEl.textContent = `${data.fps.toFixed(1)} FPS`;
                } else {
                    // 隐藏录制信息区域
                    recordingInfoEl.style.display = 'none';
                }
            } catch (error) {
                console.error('Error fetching recording stats:', error);
            }
        }

        // 更新录制状态
        async function updateStatus() {
            try {
                const response = await fetch('/record');
                const statusText = await response.text();

                if (statusText.includes('Recording')) {
                    statusEl.textContent = '正在录制';
                    statusEl.className = 'status status-recording';
                    startBtn.disabled = true;
                    stopBtn.disabled = false;

                    // 如果流媒体正在播放，停止它
                    if (streamActive) {
                        stopStream();
                        showStreamStatus('录制进行中，无法播放流媒体', true);
                    }

                    // 获取录制统计信息
                    await fetchRecordingStats();
                } else {
                    statusEl.textContent = '准备就绪';
                    statusEl.className = 'status status-stopped';
                    startBtn.disabled = false;
                    stopBtn.disabled = true;
                    recordingInfoEl.style.display = 'none';
                }
            } catch (error) {
                console.error('Error checking status:', error);
            }
        }

        // 开始录制
        async function startRecording() {
            try {
                // 如果流媒体正在播放，停止它
                if (streamActive) {
                    stopStream();
                    showStreamStatus('录制进行中，无法播放流媒体', true);
                }

                const response = await fetch('/record?action=start');
                const result = await response.text();

                if (result.includes('started')) {
                    showNotification('录制已开始');
                } else if (result.includes('streaming')) {
                    showNotification('无法在流媒体播放时录制', 5000);
                } else {
                    showNotification(result);
                }

                updateStatus();
            } catch (error) {
                showNotification('开始录制失败');
                console.error('Error starting recording:', error);
            }
        }

        // 停止录制
        async function stopRecording() {
            try {
                const response = await fetch('/record?action=stop');
                const result = await response.text();
                showNotification(result.includes('stopped') ? '录制已停止' : result);
                updateStatus();
            } catch (error) {
                showNotification('停止录制失败');
                console.error('Error stopping recording:', error);
            }
        }

        // 设置FPS
        async function setFPS(fps) {
            fpsValue.textContent = fps;
            fpsDisplay.textContent = `${fps} FPS`;

            try {
                const response = await fetch(`/fps?value=${fps}`);
                const result = await response.text();
                if (result.includes('updated')) {
                    showNotification(`帧率已设置为 ${fps} FPS`);

                    // 如果流媒体正在运行，更新帧率
                    if (streamActive) {
                        // 清除当前定时器
                        clearInterval(streamInterval);

                        // 计算新的帧间隔
                        const frameIntervalMs = 1000 / parseInt(fps);
                        console.log(`更新帧率: ${fps} FPS (间隔: ${frameIntervalMs}ms)`);

                        // 设置新的定时器
                        streamInterval = setInterval(fetchNewFrame, frameIntervalMs);
                    }
                }
            } catch (error) {
                showNotification('设置帧率失败');
                console.error('Error setting FPS:', error);
            }
        }

        // 切换MPU状态
        async function toggleMPU(enabled) {
            try {
                const response = await fetch(`/mpu_toggle?enable=${enabled ? 1 : 0}`);
                const result = await response.text();
                showNotification(enabled ? '陀螺仪数据已启用' : '陀螺仪数据已禁用');
            } catch (error) {
                showNotification('切换陀螺仪状态失败');
                console.error('Error toggling MPU:', error);
            }
        }

        // 获取当前FPS
        async function getCurrentFPS() {
            try {
                const response = await fetch('/fps');
                const result = await response.text();
                const fpsMatch = result.match(/\d+/);
                if (fpsMatch) {
                    const currentFps = fpsMatch[0];
                    fpsSlider.value = currentFps;
                    fpsValue.textContent = currentFps;
                    fpsDisplay.textContent = `${currentFps} FPS`;
                }
            } catch (error) {
                console.error('Error getting current FPS:', error);
            }
        }

        // 获取MPU状态
        async function getMPUStatus() {
            try {
                const response = await fetch('/mpu_toggle');
                const result = await response.text();
                mpuToggle.checked = result.includes('ON');
            } catch (error) {
                console.error('Error getting MPU status:', error);
            }
        }


        // 事件监听器
        startBtn.addEventListener('click', startRecording);
        stopBtn.addEventListener('click', stopRecording);

        fpsSlider.addEventListener('input', function() {
            fpsValue.textContent = this.value;
        });

        fpsSlider.addEventListener('change', function() {
            setFPS(this.value);
        });

        mpuToggle.addEventListener('change', function() {
            toggleMPU(this.checked);
        });



        // 初始化
        window.addEventListener('load', async function() {
            await updateStatus();
            await getCurrentFPS();
            await getMPUStatus();

            // 如果没有录制，自动启动流媒体
            if (statusEl.textContent !== '正在录制') {
                // 稍微延迟启动流媒体，确保其他初始化完成
                setTimeout(startStream, 500);
            }
        });

        // 页面卸载时停止流媒体
        window.addEventListener('beforeunload', function() {
            if (streamActive) {
                // 只需要清除定时器
                clearInterval(streamInterval);
                streamActive = false;
            }
        });

        // 定期更新状态
        setInterval(updateStatus, 5000);

        // 当录制时，更频繁地更新录制统计信息
        setInterval(async () => {
            if (statusEl.textContent === '正在录制') {
                await fetchRecordingStats();
            }
        }, 1000);
    </script>
</body>
</html>
)rawliteral";

static ra_filter_t *ra_filter_init(ra_filter_t *filter, size_t sample_size)
{
    memset(filter, 0, sizeof(ra_filter_t));

    filter->values = (int *)malloc(sample_size * sizeof(int));
    if (!filter->values)
    {
        return NULL;
    }
    memset(filter->values, 0, sample_size * sizeof(int));

    filter->size = sample_size;
    return filter;
}

#if ARDUHAL_LOG_LEVEL >= ARDUHAL_LOG_LEVEL_INFO
static int ra_filter_run(ra_filter_t *filter, int value)
{
    if (!filter->values)
    {
        return value;
    }
    filter->sum -= filter->values[filter->index];
    filter->values[filter->index] = value;
    filter->sum += filter->values[filter->index];
    filter->index++;
    filter->index = filter->index % filter->size;
    if (filter->count < filter->size)
    {
        filter->count++;
    }
    return filter->sum / filter->count;
}
#endif

#if CONFIG_LED_ILLUMINATOR_ENABLED
void enable_led(bool en)
{ // Turn LED On or Off
    int duty = en ? led_duty : 0;
    if (en && isStreaming && (led_duty > CONFIG_LED_MAX_INTENSITY))
    {
        duty = CONFIG_LED_MAX_INTENSITY;
    }
    ledcWrite(LED_LEDC_GPIO, duty);
    Serial.printf("[I] Set LED intensity to %d\n", duty);
}
#endif

// Redirect ESP-IDF log output to Arduino Serial
extern "C" {
    #include "esp_log.h"
}
static int arduino_log_write_v(const char *fmt, va_list args) {
    char buf[256];
    int len = vsnprintf(buf, sizeof(buf), fmt, args);
    if (len > 0) {
        Serial.print(buf);
    }
    return len;
}
static void redirect_esp_log_to_serial() {
    esp_log_set_vprintf([](const char *fmt, va_list args) -> int {
        return arduino_log_write_v(fmt, args);
    });
}

// Initialize SD card
static bool initSDCard()
{
    // Check if PSRAM is enabled and available
    if(psramFound()) {
        Serial.printf("[I] PSRAM is enabled, size: %d MB\n", ESP.getPsramSize() / (1024 * 1024));
    } else {
        Serial.printf("[W] PSRAM is not initialized! Recording performance may be limited\n");
    }

    SD_MMC.setPins(SD_MMC_CLK, SD_MMC_CMD, SD_MMC_D0);

    if (!SD_MMC.begin("/sdcard", true))
    { // Use 1-bit mode for compatibility
        Serial.printf("[E] SD Card Mount Failed\n");
        return false;
    }

    uint8_t cardType = SD_MMC.cardType();
    if (cardType == CARD_NONE)
    {
        Serial.printf("[E] No SD Card attached\n");
        return false;
    }

    Serial.printf("[I] SD Card initialized. Type: %d\n", cardType);


    return true;
}

// Helper function to generate a unique file path
String generateUniqueFilePath(const char *basePath) {
    String filePath = String(basePath);
    if (!SD_MMC.exists(filePath)) {
        return filePath; // Return the base path if no conflict
    }

    // Generate a new file name with a timestamp
    time_t now;
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        Serial.printf("[E] Failed to obtain time\n");
        return filePath; // Fallback to base path
    }

    char timestamp[32];
    strftime(timestamp, sizeof(timestamp), "_%Y%m%d_%H%M%S", &timeinfo);
    filePath = String(basePath) + String(timestamp) + ".mjpeg";

    return filePath;
}

// MPU control handler 通过访问 /mpu_toggle?enable=1 打开，/mpu_toggle?enable=0 关闭
static esp_err_t mpu_toggle_handler(httpd_req_t *req)
{
    char query[32] = {0};
    char value[8] = {0};
    bool changed = false;

    if (httpd_req_get_url_query_str(req, query, sizeof(query)) == ESP_OK) {
        if (httpd_query_key_value(query, "enable", value, sizeof(value)) == ESP_OK) {
            if (strcmp(value, "1") == 0) {
                enableMPU = true;
                changed = true;
            } else if (strcmp(value, "0") == 0) {
                enableMPU = false;
                changed = true;
            }
        }
    }
    httpd_resp_set_type(req, "text/plain");
    if (changed) {
        httpd_resp_sendstr(req, enableMPU ? "MPU enabled" : "MPU disabled");
    } else {
        httpd_resp_sendstr(req, enableMPU ? "MPU is ON" : "MPU is OFF");
    }
    return ESP_OK;
}

void recordTask(void *param) {
    size_t psram_size = ESP.getFreePsram();
    // 直接使用最多6MB作为缓冲区（如果PSRAM足够）
    size_t buffer_size = psram_size > 6291456 ? 6291456 : (psram_size > 2097152 ? 2097152 : (psram_size > 131072 ? 131072 : (psram_size > 32768 ? 32768 : 4096)));
    uint8_t* temp_buffer = NULL;

    if(psram_size > buffer_size) {
        temp_buffer = (uint8_t*)ps_malloc(buffer_size);
        Serial.printf("[I] Using PSRAM for video buffer, size: %d bytes\n", buffer_size);
    } else if(psram_size > 0) {
        temp_buffer = (uint8_t*)ps_malloc(buffer_size);
        Serial.printf("[I] Using PSRAM for video buffer, size: %d bytes\n", buffer_size);
    } else {
        temp_buffer = (uint8_t*)malloc(buffer_size);
        Serial.printf("[I] Using heap for video buffer, size: %d bytes\n", buffer_size);
    }

    if(!temp_buffer) {
        Serial.printf("[E] Failed to allocate video buffer\n");
        isRecording = false;
        vTaskDelete(NULL);
        return;
    }

    size_t buffer_used = 0;
    int totalFrameCount = 0; // 总帧数

    unsigned long lastFpsMillis = millis();
    int lastFpsFrameCount = 0;
    unsigned long lastFrameTime = 0; // Track time of last frame capture

    while (isRecording) {
        // FPS control - calculate time since last frame
        unsigned long currentTime = millis();
        unsigned long elapsedTime = currentTime - lastFrameTime;

        // If we need to wait to maintain the target FPS
        if (elapsedTime < minFrameTimeMs) {
            // Wait the remaining time to achieve target FPS
            delay(minFrameTimeMs - elapsedTime);
            currentTime = millis(); // Update current time after delay
        }

        // Record the time before capturing the frame
        lastFrameTime = currentTime;

        camera_fb_t *fb = esp_camera_fb_get();
        if (fb) {
            // Read MPU data only if enabled, otherwise fill with zeros
            MPUData mpu_data = {};
            if (enableMPU) {
                readMPUData();
                mpu_data = getMPUData();
            }

            uint64_t timestamp = esp_timer_get_time();
            // Store frame size, timestamp, and MPU data
            size_t data_size = sizeof(timestamp) + sizeof(uint32_t) + fb->len + (enableMPU ? sizeof(MPUData) : 0);

            // If adding this frame would exceed buffer, flush first
            if (buffer_used + data_size > buffer_size) {
                if (buffer_used > 0) {
                    videoFile.write(temp_buffer, buffer_used);
                    buffer_used = 0;
                }
            }

            // If frame is too large for buffer, write directly
            if (data_size > buffer_size) {
                videoFile.write((uint8_t *)&timestamp, sizeof(timestamp));
                uint32_t len = fb->len;
                videoFile.write((uint8_t *)&len, 4);
                // Write MPU data only if enabled
                if (enableMPU) {
                    videoFile.write((uint8_t *)&mpu_data, sizeof(MPUData));
                }
                videoFile.write(fb->buf, fb->len);
            } else {
                // Otherwise add to buffer
                memcpy(temp_buffer + buffer_used, (uint8_t *)&timestamp, sizeof(timestamp));
                buffer_used += sizeof(timestamp);
                uint32_t len = fb->len;
                memcpy(temp_buffer + buffer_used, (uint8_t *)&len, 4);
                buffer_used += 4;
                // Add MPU data to buffer only if enabled
                if (enableMPU) {
                    memcpy(temp_buffer + buffer_used, (uint8_t *)&mpu_data, sizeof(MPUData));
                    buffer_used += sizeof(MPUData);
                }
                memcpy(temp_buffer + buffer_used, fb->buf, fb->len);
                buffer_used += fb->len;
            }

            totalFrameCount++;
            currentFrameCount = totalFrameCount; // Update global frame count
            lastFpsFrameCount++;
            esp_camera_fb_return(fb);

            // 输出每秒帧率
            unsigned long nowFpsMillis = millis();
            if (nowFpsMillis - lastFpsMillis >= 1000) {
                currentFps = lastFpsFrameCount; // Update global FPS counter
                Serial.printf("[I] FPS: %d\n", lastFpsFrameCount);
                lastFpsFrameCount = 0;
                lastFpsMillis = nowFpsMillis;
            }

            // Flush every ~2 seconds to减少写入频率
            static uint32_t last_flush = 0;
            uint32_t now = millis();
            if (now - last_flush > 2000) {
                if (buffer_used > 0) {
                    videoFile.write(temp_buffer, buffer_used);
                    buffer_used = 0;
                }
                videoFile.flush();
                // Update current recording size
                currentRecordingSize = videoFile.size();
                last_flush = now;
            }
        }
    }

    // Final flush of any remaining data
    if (buffer_used > 0) {
        try {
            Serial.printf("[I] Final flush of %d bytes\n", buffer_used);
            videoFile.write(temp_buffer, buffer_used);
            Serial.printf("[I] Final flush completed\n");
        } catch (const std::exception& e) {
            Serial.printf("[E] Exception during final flush: %s\n", e.what());
        } catch (...) {
            Serial.printf("[E] Unknown exception during final flush\n");
        }
    }

    try {
        Serial.printf("[I] Flushing file...\n");
        videoFile.flush();
        Serial.printf("[I] File flushed\n");
    } catch (const std::exception& e) {
        Serial.printf("[E] Exception during file flush: %s\n", e.what());
    } catch (...) {
        Serial.printf("[E] Unknown exception during file flush\n");
    }

    Serial.printf("[I] Total frames saved: %d\n", totalFrameCount);

    // Free the buffer
    if (temp_buffer) {
        Serial.printf("[I] Freeing buffer memory\n");
        free(temp_buffer);
        temp_buffer = NULL;
        Serial.printf("[I] Buffer memory freed\n");
    }

    Serial.printf("[I] Recording task completed\n");
    vTaskDelete(NULL);
}

// Handler for starting/stopping recording
static esp_err_t record_handler(httpd_req_t *req)
{
    // Serial.printf("[I] Record handler called\n");

    char *buf = NULL;
    char action[32] = {0}; // Initialize to zeros

    // Parse query parameters
    size_t buf_len = httpd_req_get_url_query_len(req);
    // Serial.printf("[I] Query length: %d\n", buf_len);

    if (buf_len > 0)
    {
        buf = (char *)malloc(buf_len + 1);
        if (buf == NULL) {
            Serial.printf("[E] Failed to allocate memory for query\n");
            httpd_resp_send_500(req);
            return ESP_FAIL;
        }

        esp_err_t parse_result = httpd_req_get_url_query_str(req, buf, buf_len + 1);
        Serial.printf("[I] Parse query result: %d, query: %s\n", parse_result, buf);

        if (parse_result == ESP_OK)
        {
            esp_err_t key_result = httpd_query_key_value(buf, "action", action, sizeof(action));
            Serial.printf("[I] Parse key result: %d, action: %s\n", key_result, action);

            if (key_result == ESP_OK)
            {
                if (strcmp(action, "start") == 0)
                {
                    Serial.printf("[I] Starting recording\n");
                    // Check if already recording
                    if (isRecording) {
                        Serial.printf("[I] Already recording\n");
                        httpd_resp_set_type(req, "text/plain");
                        httpd_resp_sendstr(req, "Already recording");
                        free(buf);
                        return ESP_OK;
                    }

                    // Check if streaming is active
                    if (isStreaming)
                    {
                        Serial.printf("[I] Cannot record while streaming is active\n");
                        httpd_resp_set_type(req, "text/plain");
                        httpd_resp_sendstr(req, "Cannot record while streaming is active");
                        free(buf);
                        return ESP_OK;
                    }

                    // Generate a unique file path
                    String uniqueFilePath = generateUniqueFilePath(videoFilePath);
                    currentRecordingPath = uniqueFilePath; // Store current recording path

                    // Start recording
                    videoFile = SD_MMC.open(uniqueFilePath.c_str(), FILE_WRITE);
                    if (videoFile)
                    {
                        isRecording = true;
                        recordingStartTime = millis();
                        currentFrameCount = 0;
                        currentRecordingSize = 0;
                        currentFps = 0.0;
                        Serial.printf("[I] Started recording to %s\n", uniqueFilePath.c_str());
                        Serial.printf("Started recording to %s\r\n", uniqueFilePath.c_str()); // Added for serial output
                        xTaskCreatePinnedToCore(recordTask, "recordTask", 4096, NULL, 1, &recordTaskHandle, 1);
                        httpd_resp_set_type(req, "text/plain");
                        httpd_resp_sendstr(req, "Recording started");
                    }
                    else
                    {
                        Serial.printf("[E] Failed to open file for writing\n");
                        httpd_resp_set_type(req, "text/plain");
                        httpd_resp_sendstr(req, "Failed to open file for recording");
                    }
                }
                else if (strcmp(action, "stop") == 0)
                {
                    Serial.printf("[I] Stopping recording\n");
                    // Stop recording
                    if (isRecording) {
                        Serial.printf("[I] Setting isRecording to false\n");
                        isRecording = false;

                        // 等待录制任务完成 - 给任务一些时间来结束
                        Serial.printf("[I] Waiting for recording task to finish...\n");
                        delay(500); // 给任务一些时间来结束

                        // 安全地关闭文件
                        if (videoFile)
                        {
                            Serial.printf("[I] Closing video file...\n");
                            videoFile.flush();
                            videoFile.close();
                            Serial.printf("[I] Video file closed\n");
                        }

                        // 清理任务句柄
                        if (recordTaskHandle) {
                            Serial.printf("[I] Deleting recording task\n");
                            // 尝试安全地删除任务
                            vTaskDelete(recordTaskHandle);
                            recordTaskHandle = NULL;
                            Serial.printf("[I] Recording task deleted\n");
                        }

                        // 验证录制的文件
                        String filePath = currentRecordingPath; // 使用当前录制路径而不是基本路径
                        Serial.printf("[I] Verifying recorded file: %s\n", filePath.c_str());
                        File checkFile = SD_MMC.open(filePath.c_str(), FILE_READ);
                        if (checkFile) {
                            Serial.printf("[I] Recorded video size: %d bytes\n", checkFile.size());
                            checkFile.close();
                        } else {
                            Serial.printf("[E] Unable to open video file for verification\n");
                        }
                        httpd_resp_set_type(req, "text/plain");
                        httpd_resp_sendstr(req, "Recording stopped");
                    } else {
                        Serial.printf("[I] Not recording\n");
                        httpd_resp_set_type(req, "text/plain");
                        httpd_resp_sendstr(req, "Not recording");
                    }
                }
                else
                {
                    Serial.printf("[I] Unknown action: %s\n", action);
                    httpd_resp_set_type(req, "text/plain");
                    httpd_resp_sendstr(req, "Unknown action");
                }
            }
            else
            {
                Serial.printf("[I] Action parameter not found\n");
                httpd_resp_set_type(req, "text/plain");
                httpd_resp_sendstr(req, "Action parameter missing");
            }
        }
        else
        {
            Serial.printf("[E] Failed to parse URL query\n");
            httpd_resp_set_type(req, "text/plain");
            httpd_resp_sendstr(req, "Failed to parse URL query");
        }
        free(buf);
    }
    else
    {
        // No parameters - return status
        httpd_resp_set_type(req, "text/plain");
        if (isRecording) {
            httpd_resp_sendstr(req, "Status: Recording");
        } else {
            httpd_resp_sendstr(req, "Status: Not recording");
        }
    }

    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    return ESP_OK;
}

// Add a simple index handler to show recording controls
static esp_err_t index_handler(httpd_req_t *req) {
    httpd_resp_set_type(req, "text/html");
    return httpd_resp_send(req, RECORDING_CONTROL_HTML, strlen(RECORDING_CONTROL_HTML));
}

static size_t jpg_encode_stream(void *arg, size_t index, const void *data, size_t len)
{
    jpg_chunking_t *j = (jpg_chunking_t *)arg;
    if (!index)
    {
        j->len = 0;
    }
    if (httpd_resp_send_chunk(j->req, (const char *)data, len) != ESP_OK)
    {
        return 0;
    }
    j->len += len;
    return len;
}

static esp_err_t xclk_handler(httpd_req_t *req)
{
    // Set a fixed default XCLK value instead of parsing from request
    int xclk = 20; // Set a fixed value of 20 MHz for the XCLK
    Serial.printf("[I] Setting fixed XCLK: %d MHz\n", xclk);

    sensor_t *s = esp_camera_sensor_get();
    int res = s->set_xclk(s, LEDC_TIMER_0, xclk);
    if (res)
    {
        return httpd_resp_send_500(req);
    }

    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    return httpd_resp_send(req, NULL, 0);
}

// Handler for setting FPS
static esp_err_t fps_handler(httpd_req_t *req)
{
    char query[32] = {0};
    char value[8] = {0};
    bool changed = false;

    if (httpd_req_get_url_query_str(req, query, sizeof(query)) == ESP_OK) {
        if (httpd_query_key_value(query, "value", value, sizeof(value)) == ESP_OK) {
            int fps = atoi(value);
            if (fps > 0) {
                setTargetFPS(fps);
                changed = true;
            }
        }
    }

    httpd_resp_set_type(req, "text/plain");
    if (changed) {
        httpd_resp_sendstr(req, "FPS updated");
    } else {
        char resp[64];
        snprintf(resp, sizeof(resp), "Current FPS: %d", targetFPS);
        httpd_resp_sendstr(req, resp);
    }

    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    return ESP_OK;
}



// Handler for stopping the stream
static esp_err_t stop_stream_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/plain");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");

    if (isStreaming)
    {
        isStreaming = false;
        Serial.println("Stream stopped by client request");
        httpd_resp_sendstr(req, "Stream stopped");
    }
    else
    {
        httpd_resp_sendstr(req, "Stream was not active");
    }

    return ESP_OK;
}

// Stream handler for single image capture
static esp_err_t stream_handler(httpd_req_t *req)
{
    camera_fb_t *fb = NULL;
    esp_err_t res = ESP_OK;
    size_t _jpg_buf_len = 0;
    uint8_t *_jpg_buf = NULL;
    char query[32] = {0};

    // Check for query parameters
    if (httpd_req_get_url_query_str(req, query, sizeof(query)) == ESP_OK)
    {
        // If 'stop=1' is in the query, stop any active stream
        char value[8] = {0};
        if (httpd_query_key_value(query, "stop", value, sizeof(value)) == ESP_OK)
        {
            if (strcmp(value, "1") == 0)
            {
                isStreaming = false;
                Serial.println("Stream stopped by query parameter");
                httpd_resp_set_type(req, "text/plain");
                httpd_resp_sendstr(req, "Stream stopped");
                return ESP_OK;
            }
        }
    }

    // Check if recording is in progress
    if (isRecording)
    {
        httpd_resp_set_type(req, "text/plain");
        httpd_resp_set_status(req, "409 Conflict");
        httpd_resp_sendstr(req, "Cannot stream while recording is in progress");
        return ESP_OK;
    }

    // 不再使用isStreaming标志来防止并发请求
    // 每个请求都是独立的，可以并行处理
    Serial.println("Capturing single frame");

    // Set response type to JPEG
    httpd_resp_set_type(req, "image/jpeg");
    httpd_resp_set_hdr(req, "Content-Disposition", "inline; filename=capture.jpg");

    // Set no cache headers
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_hdr(req, "Cache-Control", "no-cache, no-store, must-revalidate");
    httpd_resp_set_hdr(req, "Pragma", "no-cache");
    httpd_resp_set_hdr(req, "Expires", "0");

    // Get frame from camera
    fb = esp_camera_fb_get();
    if (!fb)
    {
        Serial.println("Camera capture failed");
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    // If format is not JPEG, convert it
    if (fb->format != PIXFORMAT_JPEG)
    {
        bool jpeg_converted = frame2jpg(fb, 80, &_jpg_buf, &_jpg_buf_len);
        esp_camera_fb_return(fb);
        fb = NULL;

        if (!jpeg_converted)
        {
            Serial.println("JPEG compression failed");
            httpd_resp_send_500(req);
            return ESP_FAIL;
        }

        // Send the JPEG image data
        res = httpd_resp_send(req, (const char *)_jpg_buf, _jpg_buf_len);
        free(_jpg_buf);
    }
    else
    {
        // Send the JPEG image data directly
        res = httpd_resp_send(req, (const char *)fb->buf, fb->len);
        esp_camera_fb_return(fb);
    }

    // 单帧发送完成
    Serial.println("Single frame sent");

    return res;
}

// Handler for getting recording statistics
static esp_err_t recording_stats_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");

    char json_response[256];

    if (isRecording) {
        // Calculate recording duration in seconds
        unsigned long duration = (millis() - recordingStartTime) / 1000;

        // Format file size to human-readable format
        char size_str[32];
        if (currentRecordingSize < 1024) {
            snprintf(size_str, sizeof(size_str), "%u B", (unsigned int)currentRecordingSize);
        } else if (currentRecordingSize < 1024 * 1024) {
            snprintf(size_str, sizeof(size_str), "%.1f KB", currentRecordingSize / 1024.0);
        } else {
            snprintf(size_str, sizeof(size_str), "%.2f MB", currentRecordingSize / (1024.0 * 1024.0));
        }

        // Extract filename from path
        const char* filename = strrchr(currentRecordingPath.c_str(), '/');
        if (filename) {
            filename++; // Skip the slash
        } else {
            filename = currentRecordingPath.c_str();
        }

        // Create JSON response
        snprintf(json_response, sizeof(json_response),
                 "{\"recording\":true,\"duration\":%lu,\"frames\":%d,\"fps\":%.1f,\"size\":\"%s\",\"filename\":\"%s\"}",
                 duration, currentFrameCount, currentFps, size_str, filename);
    } else {
        // Not recording
        snprintf(json_response, sizeof(json_response), "{\"recording\":false}");
    }

    return httpd_resp_sendstr(req, json_response);
}


void startCameraServer()
{
    // Redirect ESP-IDF log output to Arduino Serial
    redirect_esp_log_to_serial();

    // NTP time sync initialization
    if (!sntp_enabled()) {
        sntp_setoperatingmode(SNTP_OPMODE_POLL);
        sntp_setservername(0, (char*)"ntp.aliyun.com");
        sntp_setservername(1, (char*)"pool.ntp.org");
        sntp_init();
    }

    // Wait for time to be set (max 5s)
    time_t now = 0;
    struct tm timeinfo = { 0 };
    int retry = 0;
    const int retry_count = 10;
    while (timeinfo.tm_year < (2016 - 1900) && ++retry < retry_count) {
        Serial.printf("[I] Waiting for system time to be set... (%d/%d)\n", retry, retry_count);
        vTaskDelay(500 / portTICK_PERIOD_MS);
        time(&now);
        localtime_r(&now, &timeinfo);
    }

    // Initialize SD Card
    if (!initSDCard())
    {
        Serial.printf("[E] Failed to initialize SD card - recording will be disabled\n");
    }

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.max_uri_handlers = 16;

    // Add index handler for the root path
    httpd_uri_t index_uri = {
        .uri       = "/",
        .method    = HTTP_GET,
        .handler   = index_handler,
        .user_ctx  = NULL
    };

    httpd_uri_t xclk_uri = {
        .uri = "/xclk",
        .method = HTTP_GET,
        .handler = xclk_handler,
        .user_ctx = NULL
#ifdef CONFIG_HTTPD_WS_SUPPORT
        ,
        .is_websocket = true,
        .handle_ws_control_frames = false,
        .supported_subprotocol = NULL
#endif
    };

    httpd_uri_t record_uri = {
        .uri = "/record",
        .method = HTTP_GET,
        .handler = record_handler,
        .user_ctx = NULL
#ifdef CONFIG_HTTPD_WS_SUPPORT
        ,
        .is_websocket = false, // Changed to false since this is not a websocket endpoint
        .handle_ws_control_frames = false,
        .supported_subprotocol = NULL
#endif
    };

    httpd_uri_t mpu_toggle_uri = {
        .uri = "/mpu_toggle",
        .method = HTTP_GET,
        .handler = mpu_toggle_handler,
        .user_ctx = NULL
    };

    httpd_uri_t fps_uri = {
        .uri = "/fps",
        .method = HTTP_GET,
        .handler = fps_handler,
        .user_ctx = NULL
    };



    httpd_uri_t stats_uri = {
        .uri = "/recording_stats",
        .method = HTTP_GET,
        .handler = recording_stats_handler,
        .user_ctx = NULL
    };

    httpd_uri_t stream_uri = {
        .uri = "/stream",
        .method = HTTP_GET,
        .handler = stream_handler,
        .user_ctx = NULL};

    httpd_uri_t stop_stream_uri = {
        .uri = "/stop_stream",
        .method = HTTP_GET,
        .handler = stop_stream_handler,
        .user_ctx = NULL};

    ra_filter_init(&ra_filter, 20);

    // Set initial FPS
    setTargetFPS(10); // Default to 10 FPS

    Serial.printf("[I] Starting web server on port: '%d'\n", config.server_port);
    if (httpd_start(&camera_httpd, &config) == ESP_OK)
    {
        httpd_register_uri_handler(camera_httpd, &index_uri);  // Register index handler
        httpd_register_uri_handler(camera_httpd, &xclk_uri);
        httpd_register_uri_handler(camera_httpd, &record_uri); // Register record handler
        httpd_register_uri_handler(camera_httpd, &mpu_toggle_uri); // Register MPU control handler
        httpd_register_uri_handler(camera_httpd, &fps_uri); // Register FPS control handler
        httpd_register_uri_handler(camera_httpd, &stats_uri); // Register recording stats handler
        httpd_register_uri_handler(camera_httpd, &stream_uri); // Register stream handler
        httpd_register_uri_handler(camera_httpd, &stop_stream_uri); // Register stop stream handler
        Serial.printf("[I] Camera server started successfully\n");
    } else {
        Serial.printf("[E] Failed to start camera server\n");
    }

}

// Function to set the target FPS
void setTargetFPS(int fps) {
    if (fps < 1) fps = 1; // Prevent division by zero
    if (fps > 60) fps = 60; // Cap at 60 FPS to be reasonable

    targetFPS = fps;
    minFrameTimeMs = 1000 / targetFPS;
    Serial.printf("[I] Target FPS set to %d (frame interval: %d ms)\n", targetFPS, minFrameTimeMs);
}

void setupLedFlash(int pin)
{
#if CONFIG_LED_ILLUMINATOR_ENABLED
    // Modern PWM configuration for ESP32-S3
    // Using LEDC channel 8 with 10-bit resolution and 5kHz frequency
    ledcSetup(8, 5000, 10); // channel 8, 5kHz, 10-bit resolution (0-1023)
    ledcAttachPin(pin, 8);  // attach the pin to channel 8
    // Initialize with LED off
    ledcWrite(8, 0);
#else
    Serial.printf("[I] LED flash is disabled -> CONFIG_LED_ILLUMINATOR_ENABLED = 0\n");
#endif
}