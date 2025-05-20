#ifndef __SD_READ_WRITE_H
#define __SD_READ_WRITE_H

#include "Arduino.h"
#include "FS.h"
#include "psram_buffer.h"

void listDir(fs::FS &fs, const char *dirname, uint8_t levels);
bool createDir(fs::FS &fs, const char *path);
bool removeDir(fs::FS &fs, const char *path);
void readFile(fs::FS &fs, const char *path);
void writeFile(fs::FS &fs, const char *path, const char *message);
void appendFile(fs::FS &fs, const char *path, const char *message);
void renameFile(fs::FS &fs, const char *path1, const char *path2);
void deleteFile(fs::FS &fs, const char *path);
void testFileIO(fs::FS &fs, const char *path);

// Enhanced file I/O functions using PSRAM buffer
void testFileIO_PSRAM(fs::FS &fs, const char *path);
void readFile_PSRAM(fs::FS &fs, const char *path);
void writeFile_PSRAM(fs::FS &fs, const char *path, const char *message);
void appendFile_PSRAM(fs::FS &fs, const char *path, const char *message);

#endif