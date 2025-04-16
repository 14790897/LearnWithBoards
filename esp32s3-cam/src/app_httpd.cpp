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
static const char *_STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\nX-Timestamp: %d.%06d\r\nX-IMU-Data: %f,%f,%f,%f,%f,%f\r\n\r\n";

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

static TaskHandle_t recordTaskHandle = NULL;

// Declare external variable for MPU control
extern bool enableMPU;

// FPS control variables
int targetFPS = 30; // Default target FPS
int minFrameTimeMs = 1000 / targetFPS; // Minimum time between frames in milliseconds

// HTML page with recording controls
static const char RECORDING_CONTROL_HTML[] = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>ESP32-S3-CAM Recording Control</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body { font-family: Arial, sans-serif; margin: 0; padding: 20px; text-align: center; }
        .button {
            display: inline-block;
            padding: 15px 30px;
            font-size: 24px;
            cursor: pointer;
            text-align: center;
            text-decoration: none;
            outline: none;
            color: #fff;
            border: none;
            border-radius: 10px;
            box-shadow: 0 5px #999;
            margin: 10px;
        }
        .start { background-color: #4CAF50; }
        .start:hover { background-color: #3e8e41; }
        .stop { background-color: #f44336; }
        .stop:hover { background-color: #da190b; }
        .button:active { box-shadow: 0 2px #666; transform: translateY(4px); }
        .stream {
            margin-top: 20px;
            max-width: 100%;
            border: 1px solid #ddd;
            border-radius: 4px;
            padding: 5px;
        }
        #status {
            margin-top: 20px;
            padding: 10px;
            border-radius: 5px;
            background-color: #eee;
        }
    </style>
</head>
<body>
    <h1>ESP32-S3-CAM Control</h1>
    <div>
        <a href="/record?action=start" class="button start">Start Recording</a>
        <a href="/record?action=stop" class="button stop">Stop Recording</a>
    </div>
    <div id="status">Status: Ready</div>

    <div style="margin: 20px 0;">
        <label for="fps">FPS: </label>
        <select id="fps" onchange="setFPS(this.value)">
            <option value="10">10 FPS</option>
            <option value="15">15 FPS</option>
            <option value="20">20 FPS</option>
            <option value="25">25 FPS</option>
            <option value="30" selected>30 FPS</option>
            <option value="60">60 FPS</option>
        </select>
        <span id="fpsStatus"></span>
    </div>

    <div class="stream">
        <img src="/stream" width="640" height="480" />
    </div>

    <script>
        // Simple function to update status
        async function checkStatus() {
            const response = await fetch('/record');
            const status = await response.text();
            document.getElementById('status').innerText = status;
        }

        // Function to set FPS
        async function setFPS(fps) {
            const fpsStatus = document.getElementById('fpsStatus');
            fpsStatus.innerText = 'Setting...';

            try {
                const response = await fetch(`/fps?value=${fps}`);
                const result = await response.text();
                fpsStatus.innerText = result;

                // Clear the status after 3 seconds
                setTimeout(() => {
                    fpsStatus.innerText = '';
                }, 3000);
            } catch (error) {
                fpsStatus.innerText = 'Error setting FPS';
            }
        }

        // Check current FPS on load
        async function getCurrentFPS() {
            try {
                const response = await fetch('/fps');
                const result = await response.text();
                // Extract the number from "Current FPS: XX"
                const fpsMatch = result.match(/\d+/);
                if (fpsMatch) {
                    const currentFps = fpsMatch[0];
                    document.getElementById('fps').value = currentFps;
                }
            } catch (error) {
                console.error('Error getting current FPS:', error);
            }
        }

        // Check status and FPS on load
        window.onload = function() {
            checkStatus();
            getCurrentFPS();
        };

        // Update status periodically
        setInterval(checkStatus, 5000);
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
            lastFpsFrameCount++;
            esp_camera_fb_return(fb);

            // 输出每秒帧率
            unsigned long nowFpsMillis = millis();
            if (nowFpsMillis - lastFpsMillis >= 1000) {
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
                last_flush = now;
            }
        }
    }

    // Final flush of any remaining data
    if (buffer_used > 0) {
        videoFile.write(temp_buffer, buffer_used);
    }
    videoFile.flush();
    Serial.printf("[I] Total frames saved: %d\n", totalFrameCount);

    // Free the buffer
    free(temp_buffer);
    vTaskDelete(NULL);
}

// Handler for starting/stopping recording
static esp_err_t record_handler(httpd_req_t *req)
{
    Serial.printf("[I] Record handler called\n");

    char *buf = NULL;
    char action[32] = {0}; // Initialize to zeros

    // Parse query parameters
    size_t buf_len = httpd_req_get_url_query_len(req);
    Serial.printf("[I] Query length: %d\n", buf_len);

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

                    // Generate a unique file path
                    String uniqueFilePath = generateUniqueFilePath(videoFilePath);

                    // Start recording
                    videoFile = SD_MMC.open(uniqueFilePath.c_str(), FILE_WRITE);
                    if (videoFile)
                    {
                        isRecording = true;
                        recordingStartTime = millis();
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
                        isRecording = false;
                        if (recordTaskHandle) {
                            recordTaskHandle = NULL;
                        }
                        videoFile.close();
                        // Verify video was recorded by checking the file size
                        File checkFile = SD_MMC.open(videoFilePath, FILE_READ);
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

    ra_filter_init(&ra_filter, 20);

    // Set initial FPS
    setTargetFPS(30); // Default to 30 FPS

    Serial.printf("[I] Starting web server on port: '%d'\n", config.server_port);
    if (httpd_start(&camera_httpd, &config) == ESP_OK)
    {
        httpd_register_uri_handler(camera_httpd, &index_uri);  // Register index handler
        httpd_register_uri_handler(camera_httpd, &xclk_uri);
        httpd_register_uri_handler(camera_httpd, &record_uri); // Register record handler
        httpd_register_uri_handler(camera_httpd, &mpu_toggle_uri); // Register MPU control handler
        httpd_register_uri_handler(camera_httpd, &fps_uri); // Register FPS control handler
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