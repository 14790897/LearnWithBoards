#ifndef MPU_HANDLER_H
#define MPU_HANDLER_H
#include <stdint.h>
#include <Wire.h>
#include <DFRobot_BMI160.h>
#include <Arduino.h>

// Define I2C pins for BMI160
#define I2C_SDA 21
#define I2C_SCL 47

// Structure to hold IMU data with timestamp
struct MPUData {
    uint64_t timestamp;  // microseconds
    float accel_x;
    float accel_y;
    float accel_z;
    float gyro_x;
    float gyro_y;
    float gyro_z;
    float temperature;
};

// Function declarations
void initializeMPU();
void readMPUData();
MPUData getMPUData();
void scanI2CDevices();

#endif // MPU_HANDLER_H
