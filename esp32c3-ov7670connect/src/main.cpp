#include <Arduino.h>
#include "esp_log.h"
#include "esp_camera.h"
static const char *TAG = "MAIN"; // 或其他名称
// WROVER-KIT PIN Map
#define CAM_PIN_PWDN -1  // power down is not used
#define CAM_PIN_RESET -1 // software reset will be performed
#define CAM_PIN_XCLK 11
#define CAM_PIN_PCLK 10

#define CAM_PIN_SIOD 2
#define CAM_PIN_SIOC 3

#define CAM_PIN_D7 13
#define CAM_PIN_D6 8
#define CAM_PIN_D5 9
#define CAM_PIN_D4 19
#define CAM_PIN_D3 18
#define CAM_PIN_D2 12
#define CAM_PIN_D1 1
#define CAM_PIN_D0 0
#define CAM_PIN_VSYNC 7
#define CAM_PIN_HREF 6

static camera_config_t camera_config = {
    .pin_pwdn = CAM_PIN_PWDN,
    .pin_reset = CAM_PIN_RESET,
    .pin_xclk = CAM_PIN_XCLK,
    .pin_sccb_sda = CAM_PIN_SIOD,
    .pin_sccb_scl = CAM_PIN_SIOC,

    .pin_d7 = CAM_PIN_D7,
    .pin_d6 = CAM_PIN_D6,
    .pin_d5 = CAM_PIN_D5,
    .pin_d4 = CAM_PIN_D4,
    .pin_d3 = CAM_PIN_D3,
    .pin_d2 = CAM_PIN_D2,
    .pin_d1 = CAM_PIN_D1,
    .pin_d0 = CAM_PIN_D0,
    .pin_vsync = CAM_PIN_VSYNC,
    .pin_href = CAM_PIN_HREF,
    .pin_pclk = CAM_PIN_PCLK,

    .xclk_freq_hz = 8000000, // Set to 20MHz for OV7670
    .ledc_timer = LEDC_TIMER_0,
    .ledc_channel = LEDC_CHANNEL_0,

    .pixel_format = PIXFORMAT_RGB565, // OV7670 supports RGB565
    .frame_size = FRAMESIZE_QVGA,     // OV7670 typically supports up to QVGA
    .jpeg_quality = 10,               // Adjusted for OV7670
    .fb_count = 1,                    // Use double buffering for better performance
    .grab_mode = CAMERA_GRAB_LATEST   // Use latest frame for OV7670
};
void process_image(int width, int height, pixformat_t format, uint8_t *buf, size_t len)
{
  // 打印图像信息
  Serial.printf("Image captured: %dx%d, format: %d, size: %d bytes\n", width, height, format, len);

  // 示例：保存图像数据到文件（需要文件系统支持）
  // File file = SPIFFS.open("/image.jpg", FILE_WRITE);
  // if (file)
  // {
  //     file.write(buf, len);
  //     file.close();
  //     Serial.println("Image saved to /image.jpg");
  // }

  // 示例：发送图像数据到服务器
  // send_image_to_server(buf, len);
}
esp_err_t camera_init()
{
  // power up the camera if PWDN pin is defined
  if (CAM_PIN_PWDN != -1)
  {
    pinMode(CAM_PIN_PWDN, OUTPUT);
    digitalWrite(CAM_PIN_PWDN, LOW);
  }

  // initialize the camera
  esp_err_t err = esp_camera_init(&camera_config);
  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "Camera Init Failed");
    return err;
  }

  return ESP_OK;
}

esp_err_t camera_capture()
{
  // acquire a frame
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb)
  {
    ESP_LOGE(TAG, "Camera Capture Failed");
    return ESP_FAIL;
  }
  // replace this with your own function
  process_image(fb->width, fb->height, fb->format, fb->buf, fb->len);

  // return the frame buffer back to the driver for reuse
  esp_camera_fb_return(fb);
  return ESP_OK;
}

void setup()
{
  Serial.begin(115200);
  Serial.println("Initializing camera...");
  if (camera_init() != ESP_OK)
  {
    Serial.println("Camera initialization failed!");
    while (true)
    {
      delay(1000); // Halt execution
    }
  }
  Serial.println("Camera initialized successfully.");
}

void loop()
{
  Serial.println("Capturing image...");
  if (camera_capture() != ESP_OK)
  {
    Serial.println("Image capture failed!");
  }
  else
  {
    Serial.println("Image captured successfully.");
  }
  delay(5000); // Wait 5 seconds before capturing the next image
}