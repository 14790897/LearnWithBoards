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
        
        // Check status on load
        window.onload = checkStatus;
        
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
    log_i("Set LED intensity to %d", duty);
}
#endif

// Initialize SD card
static bool initSDCard()
{
    // Check if PSRAM is enabled and available
    if(psramFound()) {
        log_i("PSRAM is enabled, size: %d MB", esp_psram_get_size() / (1024 * 1024));
    } else {
        log_w("PSRAM is not initialized! Recording performance may be limited");
    }

    SD_MMC.setPins(SD_MMC_CLK, SD_MMC_CMD, SD_MMC_D0);

    if (!SD_MMC.begin("/sdcard", true))
    { // Use 1-bit mode for compatibility
        log_e("SD Card Mount Failed");
        return false;
    }

    uint8_t cardType = SD_MMC.cardType();
    if (cardType == CARD_NONE)
    {
        log_e("No SD Card attached");
        return false;
    }

    log_i("SD Card initialized. Type: %d", cardType);


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
        log_e("Failed to obtain time");
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
    size_t buffer_size = psram_size > 0 ? 32768 : 4096; // Use larger buffer if PSRAM is available
    uint8_t* temp_buffer = NULL;
    
    if(psram_size > 0) {
        temp_buffer = (uint8_t*)ps_malloc(buffer_size);
        log_i("Using PSRAM for video buffer, size: %d bytes", buffer_size);
    } else {
        temp_buffer = (uint8_t*)malloc(buffer_size);
        log_i("Using heap for video buffer, size: %d bytes", buffer_size);
    }
    
    if(!temp_buffer) {
        log_e("Failed to allocate video buffer");
        isRecording = false;
        vTaskDelete(NULL);
        return;
    }
    
    size_t buffer_used = 0;
    
    while (isRecording) {
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
            
            esp_camera_fb_return(fb);
            
            // Flush every ~1 second to ensure data safety
            static uint32_t last_flush = 0;
            uint32_t now = millis();
            if (now - last_flush > 1000) {
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
    
    // Free the buffer
    free(temp_buffer);
    vTaskDelete(NULL);
}

// Handler for starting/stopping recording
static esp_err_t record_handler(httpd_req_t *req)
{
    log_i("Record handler called");
    
    char *buf = NULL;
    char action[32] = {0}; // Initialize to zeros

    // Parse query parameters
    size_t buf_len = httpd_req_get_url_query_len(req);
    log_i("Query length: %d", buf_len);
    
    if (buf_len > 0)
    {
        buf = (char *)malloc(buf_len + 1);
        if (buf == NULL) {
            log_e("Failed to allocate memory for query");
            httpd_resp_send_500(req);
            return ESP_FAIL;
        }
        
        esp_err_t parse_result = httpd_req_get_url_query_str(req, buf, buf_len + 1);
        log_i("Parse query result: %d, query: %s", parse_result, buf);
        
        if (parse_result == ESP_OK)
        {
            esp_err_t key_result = httpd_query_key_value(buf, "action", action, sizeof(action));
            log_i("Parse key result: %d, action: %s", key_result, action);
            
            if (key_result == ESP_OK)
            {
                if (strcmp(action, "start") == 0)
                {
                    log_i("Starting recording");
                    // Check if already recording
                    if (isRecording) {
                        log_i("Already recording");
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
                        log_i("Started recording to %s", uniqueFilePath.c_str());
                        Serial.printf("Started recording to %s\r\n", uniqueFilePath.c_str()); // Added for serial output
                        xTaskCreatePinnedToCore(recordTask, "recordTask", 4096, NULL, 1, &recordTaskHandle, 1);
                        httpd_resp_set_type(req, "text/plain");
                        httpd_resp_sendstr(req, "Recording started");
                    }
                    else
                    {
                        log_e("Failed to open file for writing");
                        httpd_resp_set_type(req, "text/plain");
                        httpd_resp_sendstr(req, "Failed to open file for recording");
                    }
                }
                else if (strcmp(action, "stop") == 0)
                {
                    log_i("Stopping recording");
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
                            log_i("Recorded video size: %d bytes", checkFile.size());
                            checkFile.close();
                        } else {
                            log_e("Unable to open video file for verification");
                        }
                        httpd_resp_set_type(req, "text/plain");
                        httpd_resp_sendstr(req, "Recording stopped");
                    } else {
                        log_i("Not recording");
                        httpd_resp_set_type(req, "text/plain");
                        httpd_resp_sendstr(req, "Not recording");
                    }
                }
                else
                {
                    log_i("Unknown action: %s", action);
                    httpd_resp_set_type(req, "text/plain");
                    httpd_resp_sendstr(req, "Unknown action");
                }
            }
            else
            {
                log_i("Action parameter not found");
                httpd_resp_set_type(req, "text/plain");
                httpd_resp_sendstr(req, "Action parameter missing");
            }
        }
        else 
        {
            log_e("Failed to parse URL query");
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
    log_i("Setting fixed XCLK: %d MHz", xclk);

    sensor_t *s = esp_camera_sensor_get();
    int res = s->set_xclk(s, LEDC_TIMER_0, xclk);
    if (res)
    {
        return httpd_resp_send_500(req);
    }

    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    return httpd_resp_send(req, NULL, 0);
}


void startCameraServer()
{
    // Initialize SD Card
    if (!initSDCard())
    {
        log_e("Failed to initialize SD card - recording will be disabled");
    }
    
    // Initialize MPU6050
    initializeMPU();

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

    ra_filter_init(&ra_filter, 20);

    log_i("Starting web server on port: '%d'", config.server_port);
    if (httpd_start(&camera_httpd, &config) == ESP_OK)
    {
        httpd_register_uri_handler(camera_httpd, &index_uri);  // Register index handler
        httpd_register_uri_handler(camera_httpd, &xclk_uri);
        httpd_register_uri_handler(camera_httpd, &record_uri); // Register record handler
        httpd_register_uri_handler(camera_httpd, &mpu_toggle_uri); // Register MPU control handler
        log_i("Camera server started successfully");
    } else {
        log_e("Failed to start camera server");
    }

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
    log_i("LED flash is disabled -> CONFIG_LED_ILLUMINATOR_ENABLED = 0");
#endif
}