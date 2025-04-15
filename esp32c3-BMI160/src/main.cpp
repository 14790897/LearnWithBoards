#include <DFRobot_BMI160.h>
#include <Arduino.h>
#include <Wire.h>

DFRobot_BMI160 bmi160;
const int8_t i2c_addr = 0x68;
// Define SDA and SCL pins for I2C communication
const int8_t SDA_PIN = 3;  // Change to your SDA pin
const int8_t SCL_PIN = 2;  // Change to your SCL pin

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

void setup()
{
  Serial.begin(115200);
  delay(100);
  
  // Initialize I2C with defined pins
  Wire.begin(SDA_PIN, SCL_PIN);
  
  // Scan for I2C devices to find BMI160 address
  scanI2CDevices();
  delay(1000);
  
  // init the hardware bmin160
  if (bmi160.softReset() != BMI160_OK)
  {
    Serial.println("reset false");
    while (1)
      ;
  }

  // set and init the bmi160 i2c address
  if (bmi160.I2cInit(i2c_addr) != BMI160_OK)
  {
    // If initialization fails with the default address, try the alternative address
    Serial.println("init failed with address 0x68, trying 0x69...");
    if (bmi160.I2cInit(0x69) != BMI160_OK) {
      Serial.println("init false");
      while (1)
        ;
    } else {
      Serial.println("BMI160 initialized successfully with address 0x69");
    }
  } else {
    Serial.println("BMI160 initialized successfully with address 0x68");
  }
}

void loop()
{
  int i = 0;
  int rslt;
  int16_t accelGyro[6] = {0};

  // get both accel and gyro data from bmi160
  // parameter accelGyro is the pointer to store the data
  rslt = bmi160.getAccelGyroData(accelGyro);
  if (rslt == 0)
  {
    for (i = 0; i < 6; i++)
    {
      if (i < 3)
      {
        // the first three are gyro data
        Serial.print(accelGyro[i] * 3.14 / 180.0);
        Serial.print("\t");
      }
      else
      {
        // the following three data are accel data
        Serial.print(accelGyro[i] / 16384.0);
        Serial.print("\t");
      }
    }
    Serial.println();
  }
  else
  {
    Serial.println("err");
  }
  delay(100);
  /*
   * //only read accel data from bmi160
   * int16_t onlyAccel[3]={0};
   * bmi160.getAccelData(onlyAccel);
   */

  /*
   * ////only read gyro data from bmi160
   * int16_t onlyGyro[3]={0};
   * bmi160.getGyroData(onlyGyro);
   */
}
