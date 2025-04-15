#include "mpu_handler.h"

// Create BMI160 object
DFRobot_BMI160 bmi160;

// Global variables to store the latest sensor readings
int16_t accelGyro[6] = {0};  // [gx, gy, gz, ax, ay, az]
uint64_t mpu_timestamp = 0;
float temperature = 0; // BMI160 may not have temperature data available through this library

// Function to scan for I2C devices
void scanI2CDevices() {
  byte error, address;
  int nDevices = 0;
  
  Serial.println("Scanning for I2C devices...");
  
  for(address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();
    
    if (error == 0) {
      Serial.print("I2C device found at address 0x");
      if (address < 16) {
        Serial.print("0");
      }
      Serial.print(address, HEX);
      Serial.println(" !");
      nDevices++;
      
      // Check if this could be the BMI160 (common addresses are 0x68 and 0x69)
      if (address == 0x68 || address == 0x69) {
        Serial.println("This might be the BMI160 sensor!");
      }
    }
    else if (error == 4) {
      Serial.print("Unknown error at address 0x");
      if (address < 16) {
        Serial.print("0");
      }
      Serial.println(address, HEX);
    }
  }
  
  if (nDevices == 0) {
    Serial.println("No I2C devices found");
  } else {
    Serial.println("Scan complete");
  }
}

void initializeMPU()
{
    // Initialize I2C for BMI160
    Wire.begin(I2C_SDA, I2C_SCL);
    
    // Scan for I2C devices to find potential BMI160 address
    // scanI2CDevices();
    
    // Try to automatically detect the BMI160 address
    bool sensor_found = false;
    byte potential_addresses[] = {0x68, 0x69};
    
    Serial.println("Attempting to detect BMI160 sensor...");
    
    // Try soft reset first
    if (bmi160.softReset() != BMI160_OK)
    {
        Serial.println("BMI160 soft reset failed - will try to continue anyway");
    }
    
    // Try all potential addresses
    for (int i = 0; i < sizeof(potential_addresses); i++) {
        Serial.print("Trying address 0x");
        Serial.print(potential_addresses[i], HEX);
        Serial.println("...");
        
        // Add retry mechanism for more reliability
        for (int retry = 0; retry < 3; retry++) {
            if (bmi160.I2cInit(potential_addresses[i]) == BMI160_OK) {
                Serial.print("BMI160 initialized successfully with address 0x");
                Serial.println(potential_addresses[i], HEX);
                sensor_found = true;
                break;
            }
            delay(100); // Short delay between retries
        }
        
        if (sensor_found) break;
    }
    
    if (!sensor_found) {
        Serial.println("BMI160 initialization failed on all addresses");
        Serial.println("Implementing fallback mode with simulated values");
        
        // We'll continue but use simulated values
        // Initialize the simulated data with some reasonable values
        accelGyro[0] = 0;  // gx
        accelGyro[1] = 0;  // gy
        accelGyro[2] = 0;  // gz
        accelGyro[3] = 0;  // ax
        accelGyro[4] = 0;  // ay
        accelGyro[5] = 16384;  // az (1g downward)
        temperature = 25.0;  // Room temperature
        
        return;
    }
    
    // Initial sensor read to populate the global variables
    readMPUData();
    
    // Print initial values to confirm it's working
    Serial.println("Initial sensor values:");
    for (int i = 0; i < 6; i++) {
        Serial.print(accelGyro[i]);
        Serial.print("\t");
    }
    Serial.println();
}

void readMPUData()
{
    // Update the timestamp
    mpu_timestamp = esp_timer_get_time();
    
    // Read BMI160 data and store in global variables
    static bool error_reported = false;
    
    int rslt = bmi160.getAccelGyroData(accelGyro);
    if (rslt != 0) {
        if (!error_reported) {
            Serial.println("Error reading sensor data - will use simulated values");
            error_reported = true;
        }
        
        // Generate simulated values with a bit of noise for testing
        static float angle = 0.0;
        angle += 0.01;
        
        // Simple simulation of sensor values
        accelGyro[0] = (int16_t)(100.0 * sin(angle));  // gx
        accelGyro[1] = (int16_t)(120.0 * cos(angle));  // gy
        accelGyro[2] = (int16_t)(80.0 * sin(angle * 2));  // gz
        accelGyro[3] = (int16_t)(100 * cos(angle));  // ax
        accelGyro[4] = (int16_t)(120 * sin(angle));  // ay
        accelGyro[5] = (int16_t)(16384 + 100 * cos(angle * 3));  // az
    }
}

// Return the latest IMU data with timestamp
MPUData getMPUData()
{
    MPUData data;
    data.timestamp = mpu_timestamp;
    
    // BMI160 library stores data as [gx, gy, gz, ax, ay, az]
    // Convert gyro data from degrees/s to radians/s (multiply by PI/180)
    data.gyro_x = accelGyro[0] * 3.14159265359 / 180.0;
    data.gyro_y = accelGyro[1] * 3.14159265359 / 180.0;
    data.gyro_z = accelGyro[2] * 3.14159265359 / 180.0;
    
    // Convert accel data from raw to g (divide by 16384.0 as per the example)
    data.accel_x = accelGyro[3] / 16384.0;
    data.accel_y = accelGyro[4] / 16384.0;
    data.accel_z = accelGyro[5] / 16384.0;
    
    // Temperature data might not be available through this library
    data.temperature = temperature;
    
    return data;
}
