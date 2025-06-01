#include <Arduino.h>
#include <BleKeyboard.h>

BleKeyboard bleKeyboard;

// --- 按钮管脚映射 ---
const int BTN_SPACE = 11; // 空格键
const int BTN_ENTER = 19; // 回车键
const int BTN_MEDIA = 9; // 媒体播放/暂停

// --- 简单消抖参数 ---
const uint16_t DEBOUNCE_MS = 25;
uint32_t lastTrig[3] = {0};

void setup()
{
  Serial.begin(115200);
  Serial.println("BLE Keyboard with real buttons");

  pinMode(BTN_SPACE, INPUT_PULLUP);
  pinMode(BTN_ENTER, INPUT_PULLUP);
  pinMode(BTN_MEDIA, INPUT_PULLUP);

  bleKeyboard.begin();
}

void sendOnce(uint8_t keycode)
{
  bleKeyboard.write(keycode);
  Serial.printf("Sent keycode 0x%02X\n", keycode);
}
void sendMediaKey(const uint8_t *key)
{
  bleKeyboard.press(key);
  delay(10);
  bleKeyboard.release(key);
  Serial.println("Sent media key");
}
void loop()
{
  if (!bleKeyboard.isConnected())
  {
    delay(100);
    return;
  }

  // 0: SPACE
  if (!digitalRead(BTN_SPACE) && millis() - lastTrig[0] > DEBOUNCE_MS)
  {
    sendOnce(' '); // 或 KEY_SPACE
    Serial.println("Sent space");
    lastTrig[0] = millis();
  }
  // 1: ENTER
  if (!digitalRead(BTN_ENTER) && millis() - lastTrig[1] > DEBOUNCE_MS)
  {
    sendOnce(KEY_RETURN);
    Serial.println("Sent enter");
    lastTrig[1] = millis();
  }
  // 2: MEDIA PLAY/PAUSE
  if (!digitalRead(BTN_MEDIA) && millis() - lastTrig[2] > DEBOUNCE_MS)
  {
    sendMediaKey(KEY_MEDIA_PLAY_PAUSE);
    Serial.println("Sent media play/pause");
    lastTrig[2] = millis();
  }
  // Serial.printf("BTN_SPACE: %d, BTN_ENTER: %d, BTN_MEDIA: %d\n",
  //   digitalRead(BTN_SPACE),
  //   digitalRead(BTN_ENTER),
  //   digitalRead(BTN_MEDIA));
}
