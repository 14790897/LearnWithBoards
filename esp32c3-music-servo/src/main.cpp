#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

// 创建 PCA9685 对象
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();

// 舵机的 PWM 范围
#define SERVOMIN 102 // 对应 0°（500μs）
#define SERVOMAX 512 // 对应 180°（2500μs）
#define SDA_PIN 4
#define SCL_PIN 5
// 初始化舵机通道
#define NUM_SERVOS 8
int servoChannels[NUM_SERVOS] = {0, 1, 2, 3, 4, 5, 6, 7}; // 对应舵机通道
float speedFactor = 0.2; // 将所有时长缩短为原来的 10%

// 舵机敲击角度
#define HIT_ANGLE 120  // 敲击角度
#define REST_ANGLE 45 // 复位角度

// 硬编码的指令字符串
const char *instructions = R"(4:449
6:449
7:1349
6:449
7:899
3:899
6:2699
7:899
4:1349
2:449
4:899
7:899
2:2699
7:899
0:1349
7:449
0:449
7:1399
7:3000
7:1000
6:1500
1:500
1:1000
6:1000
6:2000
4:500
6:500
7:1500
6:500
7:1000
3:1000
6:3000
7:1000
4:1349
2:449
4:899
7:899
2:2699
7:899
0:899
7:449
6:1399
7:899
1:1000
3:500
7:2500
7:500
6:500
4:1000
6:1000
3:1000
4:2000
7:500
1:500
3:1500
1:500
3:666
4:666
6:666
1:3000
2:1000
7:2500
1:1000
3:500
3:4000
4:500
6:500
7:1000
6:500
7:500
1:1000
7:1500
2:500
2:2000
4:1000
3:1000
1:1000
7:1000
3:1000
2:1000
6:1000
4:1000
3:2500
1:500
3:500
4:500
6:2000
4:500
6:500
7:1349
6:449
7:899
3:899
6:899
7:1799
7:899
4:1349
2:449
4:899
7:899
2:2699
7:899
0:899
7:449
6:1399
7:899
1:1000
3:500
7:2500
7:500
6:500
4:1000
6:1000
3:1000
4:4000
0:5000
0:1000
6:1000
4:1000
6:1500
7:500
7:3000
7:1000
4:1000
6:1000
0:5000
0:1000
6:1000
4:1000
6:1500
7:500
7:4000
4:3000
2:1000
2:3000
0:1000
7:4000
7:1000
6:1000
3:1000
7:1000
7:2000
6:500
4:500
2:500
1:500
2:1500
7:500
4:1000
7:1000
7:1500
5:500
5:2000
0:1500
7:500
7:6000
2:3000
2:2500
5:6000
4:500
7:500
3:3000
0:3000
2:500
6:500
3:3000
6:3000
0:500
4:500
7:1000
3:1000
0:2000
7:500
2:500
7:3000
1:3000
5:500
0:500
4:3000
7:3000
3:500
7:500
4:3000
7:3000
2:500
6:500
1:3000
4:3000
7:500
3:500
6:3000
3:3000
4:500
7:500
3:3000
0:3000
2:500
6:500
3:3000
6:3000
0:500
4:500
7:1000
3:1000
0:2000
7:500
2:500
7:3000
1:3000
5:500
0:500
4:3000
7:3000
0:500
7:500
4:3000
7:3000
5:500
0:500
4:1000
7:1000
7:500
3:500
6:1000
3:1000
4:500
7:500
3:3000
0:3000
7:500
3:500
6:3000
3:3000
6:500
1:500
6:3000
2:3000
4:500
7:500
3:3000
0:3000
2:500
6:500
3:3000
6:3000
0:500
4:500
7:1000
4:1000
0:500
2:500
6:1000
1:1000
7:500
2:500
7:3000
3:3000
5:500
0:500
4:3000
7:3000
2:500
6:500
1:3000
4:3000
3:500
6:500
3:3000
7:3000
7:500
3:500
6:3000
3:3000
4:500
7:500
3:3000
0:3000
2:500
6:500
3:3000
6:3000
0:500
4:500
7:1000
3:1000
0:2000
7:500
2:500
7:3000
1:3000
5:500
0:500
4:3000
7:3000
0:500
7:500
4:3000
7:3000
5:500
0:500
4:1000
7:1000
7:500
3:500
6:1000
3:1000
4:500
0:500
3:3000
0:3000
1:8000
5:8000
0:8000
0:8000
3:8000
0:8000
1:8000
5:8000
0:8000
0:6000
3:6000
0:6000
7:500
1:500
3:500
3:3000
1:1000
1:4000
5:4000
0:4000
3:4000
7:8000
2:8000
4:2000
7:2000
3:2000
6:2000
2:2000
0:2000
3:2000
6:2000
2:2000
4:2000
0:2000
3:2000
5:4000
0:4000
3:4000
6:4000
2:4000
5:4000
7:4000
2:4000
5:4000
4:4000
3:7500

)";

// 设置舵机角度函数
void setServoAngle(uint8_t channel, uint16_t angle)
{
    uint16_t pulse = map(angle, 0, 180, SERVOMIN, SERVOMAX);
    pwm.setPWM(channel, 0, pulse);
}

// 敲击琴键函数
void hitKey(int servoChannel)
{
    if (servoChannel < 0 || servoChannel >= NUM_SERVOS)
        return;

    setServoAngle(servoChannel, HIT_ANGLE);
    delay(200); // 敲击时间
    setServoAngle(servoChannel, REST_ANGLE);
}

// 初始化系统
void setup()
{
    Serial.begin(115200);
    Wire.begin(SDA_PIN, SCL_PIN);

    // 初始化 PCA9685
    pwm.begin();
    pwm.setPWMFreq(50); // 设置 PWM 频率为 50Hz

    // 初始化舵机位置
    for (int i = 0; i < NUM_SERVOS; i++)
    {
        setServoAngle(servoChannels[i], REST_ANGLE);
    }

    Serial.println("系统初始化完成！");
}

// 解析字符串并控制舵机
void loop()
{
    // 将硬编码的字符串解析为单行指令
    String instructionString = instructions;
    int currentIndex = 0;

    while (currentIndex < instructionString.length())
    {
        int lineEndIndex = instructionString.indexOf('\n', currentIndex);
        if (lineEndIndex == -1)
        {
            lineEndIndex = instructionString.length();
        }

        String line = instructionString.substring(currentIndex, lineEndIndex);
        currentIndex = lineEndIndex + 1;

        if (line.length() > 0)
        {
            int colonIndex = line.indexOf(':');
            int servoChannel = line.substring(0, colonIndex).toInt();
            int duration = line.substring(colonIndex + 1).toInt();

            // 敲击琴键
            hitKey(servoChannel);
            // delay(duration); // 等待时长
            delay(duration * speedFactor);
        }
    }



}
