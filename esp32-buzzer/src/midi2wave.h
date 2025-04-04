#ifndef MIDI2WAVE_H
#define MIDI2WAVE_H

#include <Arduino.h>
#include "midi_data.h" // 引入 MIDI 数据文件

#define MAX_NOTE 128
#define KEYBUF_SIZE 4
#define SONG_LEN 712
#define TEMPO 0.6103515625

// 全局变量声明
extern unsigned char key_vels[MAX_NOTE];
extern char active_keys[KEYBUF_SIZE];
extern short event_length;
extern char note_count;

// MIDI 数据访问宏
#define NOTE_NUMBER(ptr) pgm_read_word(&notes[ptr])         // 从 notes[] 读取音符
#define NOTE_DELAY(ptr) (pgm_read_dword(&params[ptr]) >> 8) // 高位表示延时
#define NOTE_VEL(ptr) (pgm_read_dword(&params[ptr]) & 0xFF) // 低位表示力度

void setupMidi();
void renderWaveBuffer();
void loadNextEvent();

#endif