#ifndef MPU_HANDLER_H
#define MPU_HANDLER_H

#define sensor_t mpu_sensor_t // 重命名 mpu 的 sensor_t
#include <Adafruit_MPU6050.h>
#undef sensor_t // 恢复原始定义
#include <Adafruit_Sensor.h>

// Define I2C pins for ESP32-S3
#define I2C_SDA 21
#define I2C_SCL 47

void initializeMPU();
void readMPUData();

#endif // MPU_HANDLER_H
