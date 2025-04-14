#include "mpu_handler.h"
#include <Wire.h>
#include <Arduino.h>

// Create MPU6050 object
Adafruit_MPU6050 mpu;

void initializeMPU()
{
    // Initialize I2C for MPU6050
    Wire.begin(I2C_SDA, I2C_SCL);

    if (!mpu.begin())
    {
        Serial.println("Failed to find MPU6050 chip");
        while (1)
        {
            delay(10);
        }
    }
    // Initialize MPU6050

    Serial.println("MPU6050 Found!");

    // Set accelerometer range
    mpu.setAccelerometerRange(MPU6050_RANGE_8_G);

    // Set gyro range
    mpu.setGyroRange(MPU6050_RANGE_500_DEG);

    // Set filter bandwidth
    mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
}

void readMPUData()
{
    // Read MPU6050 data
    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);
    Serial.print("Accelerometer: ");
    Serial.print("X: ");
    Serial.print(a.acceleration.x);
}
