/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sdkconfig.h"
#include "esp_attr.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_cache.h"
#include "driver/ledc.h"
#include "esp_camera.h"
#include "esp_lcd_ili9341.h"
#include "esp_heap_caps.h"
#include "esp_system.h"
#include "example_config.h"

// Alternative SPI host definition - uncomment to try SPI2_HOST instead
// #define LCD_SPI_HOST SPI3_HOST
#define LCD_SPI_HOST SPI2_HOST

static const char *TAG = "dvp_camera_spi";

// Camera initialization function for ESP32-S3
static esp_err_t example_camera_init(void)
{
    ESP_LOGI(TAG, "=== Camera Initialization ===");

    camera_config_t config;
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;
    config.pin_d0 = EXAMPLE_ISP_DVP_CAM_D0_IO;
    config.pin_d1 = EXAMPLE_ISP_DVP_CAM_D1_IO;
    config.pin_d2 = EXAMPLE_ISP_DVP_CAM_D2_IO;
    config.pin_d3 = EXAMPLE_ISP_DVP_CAM_D3_IO;
    config.pin_d4 = EXAMPLE_ISP_DVP_CAM_D4_IO;
    config.pin_d5 = EXAMPLE_ISP_DVP_CAM_D5_IO;
    config.pin_d6 = EXAMPLE_ISP_DVP_CAM_D6_IO;
    config.pin_d7 = EXAMPLE_ISP_DVP_CAM_D7_IO;
    config.pin_xclk = EXAMPLE_ISP_DVP_CAM_XCLK_IO;
    config.pin_pclk = EXAMPLE_ISP_DVP_CAM_PCLK_IO;
    config.pin_vsync = EXAMPLE_ISP_DVP_CAM_VSYNC_IO;
    config.pin_href = EXAMPLE_ISP_DVP_CAM_HSYNC_IO;
    config.pin_sccb_sda = EXAMPLE_ISP_DVP_CAM_SCCB_SDA_IO;
    config.pin_sccb_scl = EXAMPLE_ISP_DVP_CAM_SCCB_SCL_IO;
    config.pin_pwdn = EXAMPLE_ISP_DVP_CAM_PWDN_IO;
    config.pin_reset = EXAMPLE_ISP_DVP_CAM_RESET_IO;
    config.xclk_freq_hz = 20000000;
    config.frame_size = FRAMESIZE_QVGA; // 320x240
    config.pixel_format = PIXFORMAT_RGB565; // RGB565 format
    config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
    config.fb_location = CAMERA_FB_IN_PSRAM; // 使用 PSRAM，因为已经成功初始化
    config.jpeg_quality = 12;
    config.fb_count = 2; // 使用 2 个缓冲提高性能

    // Camera init
    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Camera init failed with error 0x%x", err);
        return err;
    }

    // Get camera sensor
    sensor_t *s = esp_camera_sensor_get();
    if (s == NULL) {
        ESP_LOGE(TAG, "Failed to get camera sensor");
        return ESP_FAIL;
    }

    // OV7670 specific settings
    s->set_brightness(s, 0);     // -2 to 2
    s->set_contrast(s, 0);       // -2 to 2
    s->set_saturation(s, 0);     // -2 to 2
    s->set_gainceiling(s, GAINCEILING_128X);
    s->set_colorbar(s, 0);       // 0 = disable, 1 = enable
    s->set_whitebal(s, 1);       // 0 = disable, 1 = enable
    s->set_gain_ctrl(s, 1);      // 0 = disable, 1 = enable
    s->set_exposure_ctrl(s, 1);  // 0 = disable, 1 = enable
    s->set_hmirror(s, 0);        // 0 = disable, 1 = enable
    s->set_vflip(s, 0);          // 0 = disable, 1 = enable    
    ESP_LOGI(TAG, "Camera initialized successfully");
    
    // 显示摄像头内存使用情况
    multi_heap_info_t psram_info_after;
    heap_caps_get_info(&psram_info_after, MALLOC_CAP_SPIRAM);
    ESP_LOGI(TAG, "After camera init - PSRAM free: %zu bytes", psram_info_after.total_free_bytes);
    
    return ESP_OK;
}
static esp_err_t example_spi_lcd_init(esp_lcd_panel_handle_t *panel_handle, void **frame_buffer)
{
    esp_err_t ret = ESP_OK;
    
    // Initialize SPI bus
    spi_bus_config_t buscfg = {
        .sclk_io_num = EXAMPLE_PIN_NUM_SCLK,
        .mosi_io_num = EXAMPLE_PIN_NUM_MOSI,
        .miso_io_num = EXAMPLE_PIN_NUM_MISO,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = EXAMPLE_LCD_H_RES * 80 * sizeof(uint16_t),
    };
    ESP_ERROR_CHECK(spi_bus_initialize(LCD_SPI_HOST, &buscfg, SPI_DMA_CH_AUTO));

    esp_lcd_panel_io_handle_t io_handle = NULL;
    esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = EXAMPLE_PIN_NUM_LCD_DC,
        .cs_gpio_num = EXAMPLE_PIN_NUM_LCD_CS,
        .pclk_hz = EXAMPLE_LCD_PIXEL_CLOCK_HZ,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
        .spi_mode = 0,
        .trans_queue_depth = 10,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_SPI_HOST, &io_config, &io_handle));

    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = EXAMPLE_PIN_NUM_LCD_RST,
        .rgb_endian = LCD_RGB_ENDIAN_BGR,
        .bits_per_pixel = 16,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_ili9341(io_handle, &panel_config, panel_handle));

    ESP_ERROR_CHECK(esp_lcd_panel_reset(*panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(*panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_invert_color(*panel_handle, true));

    // Configure backlight
    gpio_config_t bk_gpio_config = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = 1ULL << EXAMPLE_PIN_NUM_BK_LIGHT
    };
    ESP_ERROR_CHECK(gpio_config(&bk_gpio_config));
    ESP_ERROR_CHECK(gpio_set_level(EXAMPLE_PIN_NUM_BK_LIGHT, EXAMPLE_LCD_BK_LIGHT_ON_LEVEL));
    
    // Allocate frame buffer in PSRAM for larger capacity
    size_t buffer_size = EXAMPLE_LCD_H_RES * EXAMPLE_LCD_V_RES * sizeof(uint16_t);
    *frame_buffer = heap_caps_malloc(buffer_size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (*frame_buffer == NULL) {
        ESP_LOGE(TAG, "Failed to allocate frame buffer in PSRAM");
        return ESP_ERR_NO_MEM;
    }

    return ret;
}

// ILI9341 LCD initialization function

void app_main(void)
{
    esp_lcd_panel_handle_t panel_handle = NULL;
    void *frame_buffer = NULL;
    size_t frame_buffer_size = 0;
    
    // 检查PSRAM状态
    ESP_LOGI(TAG, "=== PSRAM Status Check ===");
    
    // 检查PSRAM是否可用
    multi_heap_info_t psram_info;
    heap_caps_get_info(&psram_info, MALLOC_CAP_SPIRAM);
    
    if (psram_info.total_allocated_bytes + psram_info.total_free_bytes > 0) {
        ESP_LOGI(TAG, "PSRAM is available");
        ESP_LOGI(TAG, "PSRAM total: %zu bytes (%.2f MB)", 
                 psram_info.total_allocated_bytes + psram_info.total_free_bytes,
                 (float)(psram_info.total_allocated_bytes + psram_info.total_free_bytes) / (1024 * 1024));
        ESP_LOGI(TAG, "PSRAM free: %zu bytes (%.2f MB)", 
                 psram_info.total_free_bytes, 
                 (float)psram_info.total_free_bytes / (1024 * 1024));
        ESP_LOGI(TAG, "PSRAM used: %zu bytes (%.2f MB)", 
                 psram_info.total_allocated_bytes,
                 (float)psram_info.total_allocated_bytes / (1024 * 1024));
    } else {
        ESP_LOGE(TAG, "PSRAM is NOT available or initialized");
    }
    
    // 显示内部RAM状态
    multi_heap_info_t dram_info;
    heap_caps_get_info(&dram_info, MALLOC_CAP_INTERNAL);
    ESP_LOGI(TAG, "Internal RAM free: %zu bytes (%.2f KB)", dram_info.total_free_bytes, (float)dram_info.total_free_bytes / 1024);
    ESP_LOGI(TAG, "=== End PSRAM Status ===");

    /**
     * @background
     * OV7670 sensor outputs RGB565 format
     * Direct display on ILI9341 320x240 SPI LCD without ISP processing
     */
    
    //---------------SPI LCD Init------------------//
    ESP_ERROR_CHECK(example_spi_lcd_init(&panel_handle, &frame_buffer));

    //---------------Necessary variable config------------------//
    frame_buffer_size = EXAMPLE_LCD_H_RES * EXAMPLE_LCD_V_RES * EXAMPLE_RGB565_BITS_PER_PIXEL / 8;

    ESP_LOGI(TAG, "LCD_H_RES: %d, LCD_V_RES: %d, bits per pixel: %d", EXAMPLE_LCD_H_RES, EXAMPLE_LCD_V_RES, EXAMPLE_RGB565_BITS_PER_PIXEL);
    ESP_LOGI(TAG, "frame_buffer_size: %zu", frame_buffer_size);
    ESP_LOGI(TAG, "frame_buffer: %p", frame_buffer);

    // =================================================================
    // 阶段1: 测试LCD显示功能（先不连接摄像头）
    // =================================================================
    ESP_LOGI(TAG, "=== Phase 1: Testing LCD Display ===");

    //---------------LCD Display Test------------------//
    ESP_LOGI(TAG, "Testing LCD with color patterns...");

    // Test 1: 全白屏
    ESP_LOGI(TAG, "Test 1: White screen");
    memset(frame_buffer, 0xFF, frame_buffer_size);
    esp_cache_msync((void *)frame_buffer, frame_buffer_size, ESP_CACHE_MSYNC_FLAG_DIR_C2M);
    esp_lcd_panel_draw_bitmap(panel_handle, 0, 0, EXAMPLE_LCD_H_RES, EXAMPLE_LCD_V_RES, frame_buffer);
    vTaskDelay(pdMS_TO_TICKS(2000));

    // Test 2: 全黑屏
    ESP_LOGI(TAG, "Test 2: Black screen");
    memset(frame_buffer, 0x00, frame_buffer_size);
    esp_cache_msync((void *)frame_buffer, frame_buffer_size, ESP_CACHE_MSYNC_FLAG_DIR_C2M);
    esp_lcd_panel_draw_bitmap(panel_handle, 0, 0, EXAMPLE_LCD_H_RES, EXAMPLE_LCD_V_RES, frame_buffer);
    vTaskDelay(pdMS_TO_TICKS(2000));

    // Test 3: 红色屏幕 (RGB565: 0xF800)
    ESP_LOGI(TAG, "Test 3: Red screen");
    uint16_t *pixel_buffer = (uint16_t *)frame_buffer;
    for (int i = 0; i < EXAMPLE_LCD_H_RES * EXAMPLE_LCD_V_RES; i++)
    {
        pixel_buffer[i] = 0xF800; // Red in RGB565
    }
    esp_cache_msync((void *)frame_buffer, frame_buffer_size, ESP_CACHE_MSYNC_FLAG_DIR_C2M);
    esp_lcd_panel_draw_bitmap(panel_handle, 0, 0, EXAMPLE_LCD_H_RES, EXAMPLE_LCD_V_RES, frame_buffer);
    vTaskDelay(pdMS_TO_TICKS(2000));

    // Test 4: 绿色屏幕 (RGB565: 0x07E0)
    ESP_LOGI(TAG, "Test 4: Green screen");
    for (int i = 0; i < EXAMPLE_LCD_H_RES * EXAMPLE_LCD_V_RES; i++)
    {
        pixel_buffer[i] = 0x07E0; // Green in RGB565
    }
    esp_cache_msync((void *)frame_buffer, frame_buffer_size, ESP_CACHE_MSYNC_FLAG_DIR_C2M);
    esp_lcd_panel_draw_bitmap(panel_handle, 0, 0, EXAMPLE_LCD_H_RES, EXAMPLE_LCD_V_RES, frame_buffer);
    vTaskDelay(pdMS_TO_TICKS(2000));

    // Test 5: 蓝色屏幕 (RGB565: 0x001F)
    ESP_LOGI(TAG, "Test 5: Blue screen");
    for (int i = 0; i < EXAMPLE_LCD_H_RES * EXAMPLE_LCD_V_RES; i++)
    {
        pixel_buffer[i] = 0x001F; // Blue in RGB565
    }
    esp_cache_msync((void *)frame_buffer, frame_buffer_size, ESP_CACHE_MSYNC_FLAG_DIR_C2M);
    esp_lcd_panel_draw_bitmap(panel_handle, 0, 0, EXAMPLE_LCD_H_RES, EXAMPLE_LCD_V_RES, frame_buffer);
    vTaskDelay(pdMS_TO_TICKS(2000));

    // Test 6: 彩色条纹测试
    ESP_LOGI(TAG, "Test 6: Color bars");
    for (int y = 0; y < EXAMPLE_LCD_V_RES; y++)
    {
        for (int x = 0; x < EXAMPLE_LCD_H_RES; x++)
        {
            uint16_t color;
            if (x < EXAMPLE_LCD_H_RES / 7)
            {
                color = 0xF800; // Red
            }
            else if (x < 2 * EXAMPLE_LCD_H_RES / 7)
            {
                color = 0xFFE0; // Yellow
            }
            else if (x < 3 * EXAMPLE_LCD_H_RES / 7)
            {
                color = 0x07E0; // Green
            }
            else if (x < 4 * EXAMPLE_LCD_H_RES / 7)
            {
                color = 0x07FF; // Cyan
            }
            else if (x < 5 * EXAMPLE_LCD_H_RES / 7)
            {
                color = 0x001F; // Blue
            }
            else if (x < 6 * EXAMPLE_LCD_H_RES / 7)
            {
                color = 0xF81F; // Magenta
            }
            else
            {
                color = 0xFFFF; // White
            }
            pixel_buffer[y * EXAMPLE_LCD_H_RES + x] = color;
        }
    }
    esp_cache_msync((void *)frame_buffer, frame_buffer_size, ESP_CACHE_MSYNC_FLAG_DIR_C2M);
    esp_lcd_panel_draw_bitmap(panel_handle, 0, 0, EXAMPLE_LCD_H_RES, EXAMPLE_LCD_V_RES, frame_buffer);

    ESP_LOGI(TAG, "LCD Display Tests Complete. Check if all colors display correctly.");
    ESP_LOGI(TAG, "Waiting 5 seconds before starting camera test...");
    vTaskDelay(pdMS_TO_TICKS(5000));

    // =================================================================
    // 阶段2: 测试摄像头功能（不显示到LCD）
    // =================================================================
    ESP_LOGI(TAG, "=== Phase 2: Testing Camera Capture ===");

    //--------Camera Init-----------//
    ESP_ERROR_CHECK(example_camera_init());

    // 摄像头测试 - 只获取图像，不显示
    ESP_LOGI(TAG, "Testing camera capture (without LCD display)...");

    uint32_t camera_test_frames = 10;
    for (uint32_t i = 0; i < camera_test_frames; i++)
    {
        camera_fb_t *pic = esp_camera_fb_get();
        if (pic)
        {
            ESP_LOGI(TAG, "Camera frame %lu: format=%d, width=%d, height=%d, len=%d",
                     i + 1, pic->format, pic->width, pic->height, pic->len);

            // 检查图像格式和尺寸
            if (pic->format == PIXFORMAT_RGB565)
            {
                ESP_LOGI(TAG, "✓ Correct format: RGB565");
            }
            else
            {
                ESP_LOGW(TAG, "✗ Unexpected format: %d (expected RGB565)", pic->format);
            }

            if (pic->width == EXAMPLE_LCD_H_RES && pic->height == EXAMPLE_LCD_V_RES)
            {
                ESP_LOGI(TAG, "✓ Correct resolution: %dx%d", pic->width, pic->height);
            }
            else
            {
                ESP_LOGW(TAG, "✗ Unexpected resolution: %dx%d (expected %dx%d)",
                         pic->width, pic->height, EXAMPLE_LCD_H_RES, EXAMPLE_LCD_V_RES);
            }

            if (pic->len == frame_buffer_size)
            {
                ESP_LOGI(TAG, "✓ Correct buffer size: %d bytes", pic->len);
            }
            else
            {
                ESP_LOGW(TAG, "✗ Unexpected buffer size: %d bytes (expected %zu)", pic->len, frame_buffer_size);
            }

            esp_camera_fb_return(pic);
        }
        else
        {
            ESP_LOGE(TAG, "✗ Camera capture failed on frame %lu", i + 1);
        }

        vTaskDelay(pdMS_TO_TICKS(500)); // 0.5秒间隔
    }

    ESP_LOGI(TAG, "Camera test complete. Check logs above for any issues.");
    ESP_LOGI(TAG, "=== Both tests completed. Ready for integration if both passed. ===");

    // 恢复到白屏等待
    memset(frame_buffer, 0xFF, frame_buffer_size);
    esp_cache_msync((void *)frame_buffer, frame_buffer_size, ESP_CACHE_MSYNC_FLAG_DIR_C2M);
    esp_lcd_panel_draw_bitmap(panel_handle, 0, 0, EXAMPLE_LCD_H_RES, EXAMPLE_LCD_V_RES, frame_buffer);

    // 程序结束，保持运行状态
    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

