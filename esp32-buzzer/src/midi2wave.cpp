#include <Arduino.h>
#include "midi2wave.h"

unsigned char key_vels[MAX_NOTE];
char active_keys[KEYBUF_SIZE];
short event_length;
char note_count;

const int LED_PINS[] = {2, 3, 4, 5, 6, 7, 8};
short ptr;

void setupMidi()
{
    for (int i = 0; i < MAX_NOTE; ++i)
    {
        key_vels[i] = 0;
    }
    for (char i = 0; i < KEYBUF_SIZE; ++i)
    {
        active_keys[i] = 0;
    }
    ptr = 0;

    for (int i = 0; i < 7; ++i)
    {
        pinMode(LED_PINS[i], OUTPUT);
        digitalWrite(LED_PINS[i], LOW);
    }
}

void renderWaveBuffer()
{
    unsigned char leds = 0;
    note_count = 0;

    // 清除所有非活动音符
    for (char i = 0; i < KEYBUF_SIZE; ++i)
    {
        if (active_keys[i] > 0 && key_vels[active_keys[i] - 1] == 0)
        {
            active_keys[i] = 0;
        }
    }

    // 添加新的活动音符
    for (int i = 0; i < MAX_NOTE; ++i)
    {
        if (key_vels[i] > 0)
        {
            bool found = false;
            for (char j = 0; j < KEYBUF_SIZE; ++j)
            {
                if (active_keys[j] == i + 1)
                {
                    found = true;
                    break;
                }
            }
            if (!found)
            {
                for (char j = 0; j < KEYBUF_SIZE; ++j)
                {
                    if (active_keys[j] == 0)
                    {
                        active_keys[j] = i + 1;
                        note_count++;
                        break;
                    }
                }
            }
        }
    }

    // 更新LED显示
    for (char i = 0; i < KEYBUF_SIZE; ++i)
    {
        if (active_keys[i] > 0)
        {
            leds |= 1 << (active_keys[i] % 7);
        }
    }

    for (int i = 0; i < 7; ++i)
    {
        digitalWrite(LED_PINS[i], (leds & (1 << i)) ? HIGH : LOW);
    }
}

void loadNextEvent()
{
    if (ptr >= SONG_LEN)
    {
        Serial.println("Ended.");
        setupMidi();
        event_length = 3000;
        for (int i = 0; i < 7; ++i)
        {
            digitalWrite(LED_PINS[i], HIGH);
        }
        return;
    }

    int new_length = NOTE_DELAY(ptr) * TEMPO;
    unsigned char note = NOTE_NUMBER(ptr);
    unsigned char vel = NOTE_VEL(ptr);

    // 确保音符在有效范围内
    if (note > 0 && note <= 127) {
        key_vels[note - 1] = vel;
        Serial.printf("Note: %d, Velocity: %d\n", note, vel);
    }

    ++ptr;
    if (new_length == 0)
    {
        loadNextEvent();
    }
    else
    {
        renderWaveBuffer();
        event_length = new_length;
    }
}
