#include "sd_read_write.h"
#include "esp_task_wdt.h"

// Global PSRAM buffer for file operations
PSRAMBuffer g_psramBuffer;

void listDir(fs::FS &fs, const char *dirname, uint8_t levels)
{
  Serial.printf("Listing directory: %s\n", dirname);

  File root = fs.open(dirname);
  if (!root)
  {
    Serial.println("Failed to open directory");
    return;
  }
  if (!root.isDirectory())
  {
    Serial.println("Not a directory");
    return;
  }

  File file = root.openNextFile();
  while (file)
  {
    if (file.isDirectory())
    {
      Serial.print("  DIR : ");
      Serial.println(file.name());
      if (levels)
      {
        listDir(fs, file.path(), levels - 1);
      }
    }
    else
    {
      Serial.print("  FILE: ");
      Serial.print(file.name());
      Serial.print("  SIZE: ");
      Serial.println(file.size());
    }
    file = root.openNextFile();
  }
}

bool removeDir(fs::FS &fs, const char *path)
{
  return fs.rmdir(path); // rmdir 返回 bool
}
bool createDir(fs::FS &fs, const char *path)
{
  return fs.mkdir(path); // mkdir 返回 bool
}

void readFile(fs::FS &fs, const char *path)
{
  Serial.printf("Reading file: %s\n", path);

  File file = fs.open(path);
  if (!file)
  {
    Serial.println("Failed to open file for reading");
    return;
  }

  Serial.print("Read from file: ");
  while (file.available())
  {
    Serial.write(file.read());
  }
}

void writeFile(fs::FS &fs, const char *path, const char *message)
{
  Serial.printf("Writing file: %s\n", path);

  File file = fs.open(path, FILE_WRITE);
  if (!file)
  {
    Serial.println("Failed to open file for writing");
    return;
  }
  if (file.print(message))
  {
    Serial.println("File written");
  }
  else
  {
    Serial.println("Write failed");
  }
}

void appendFile(fs::FS &fs, const char *path, const char *message)
{
  Serial.printf("Appending to file: %s\n", path);

  File file = fs.open(path, FILE_APPEND);
  if (!file)
  {
    Serial.println("Failed to open file for appending");
    return;
  }
  if (file.print(message))
  {
    Serial.println("Message appended");
  }
  else
  {
    Serial.println("Append failed");
  }
}

void renameFile(fs::FS &fs, const char *path1, const char *path2)
{
  Serial.printf("Renaming file %s to %s\n", path1, path2);
  if (fs.rename(path1, path2))
  {
    Serial.println("File renamed");
  }
  else
  {
    Serial.println("Rename failed");
  }
}

void deleteFile(fs::FS &fs, const char *path)
{
  Serial.printf("Deleting file: %s\n", path);
  if (fs.remove(path))
  {
    Serial.println("File deleted");
  }
  else
  {
    Serial.println("Delete failed");
  }
}

void testFileIO(fs::FS &fs, const char *path)
{
  // 重置看门狗计时器
  esp_task_wdt_reset();

  File file = fs.open(path);
  static uint8_t buf[512];
  size_t len = 0;
  uint32_t start = millis();
  uint32_t end = start;
  uint32_t lastWdtReset = start;

  if (file)
  {
    len = file.size();
    size_t flen = len;
    start = millis();
    lastWdtReset = start;

    Serial.printf("Starting standard read test with file size: %u bytes\n", flen);

    while (len)
    {
      // 每秒重置一次看门狗计时器
      uint32_t now = millis();
      if (now - lastWdtReset > 1000)
      {
        esp_task_wdt_reset();
        lastWdtReset = now;
      }

      size_t toRead = len;
      if (toRead > 512)
      {
        toRead = 512;
      }
      file.read(buf, toRead);
      len -= toRead;
      yield(); // 让出CPU，防止WDT超时
    }
    end = millis() - start;
    Serial.printf("%u bytes read for %u ms (%.2f KB/s)\r\n",
                  flen, end, flen / (float)end);
    file.close();
  }
  else
  {
    Serial.println("Failed to open file for reading");
  }

  // 重置看门狗计时器
  esp_task_wdt_reset();

  // 使用较小的测试文件以避免超时
  size_t testSize = 1 * 1024 * 1024; // 1MB
  size_t writeCount = testSize / 512;

  Serial.printf("Starting standard write test with size: %u bytes\n", testSize);

  file = fs.open(path, FILE_WRITE);
  if (!file)
  {
    Serial.println("Failed to open file for writing");
    return;
  }

  // 填充随机数据
  for (size_t i = 0; i < 512; i++)
  {
    buf[i] = random(0, 255);
  }

  size_t i;
  start = millis();
  lastWdtReset = start;

  for (i = 0; i < writeCount; i++)
  {
    // 每秒重置一次看门狗计时器
    uint32_t now = millis();
    if (now - lastWdtReset > 1000)
    {
      esp_task_wdt_reset();
      lastWdtReset = now;
      Serial.print("."); // 显示进度
    }

    file.write(buf, 512);
    yield(); // 让出CPU，防止WDT超时
  }
  end = millis() - start;
  Serial.printf("\n%u bytes written for %u ms (%.2f KB/s)\n",
                i * 512, end, (i * 512) / (float)end);
  file.close();

  // 最后再次重置看门狗计时器
  esp_task_wdt_reset();
}

// Enhanced file I/O functions using PSRAM buffer
void readFile_PSRAM(fs::FS &fs, const char *path)
{
  Serial.printf("Reading file with PSRAM buffer: %s\n", path);

  // Initialize PSRAM buffer if not already done
  if (!g_psramBuffer.isInitialized())
  {
    if (!g_psramBuffer.init())
    {
      Serial.println("Failed to initialize PSRAM buffer, falling back to standard function");
      readFile(fs, path);
      return;
    }
    Serial.printf("PSRAM buffer initialized: %u bytes\n", g_psramBuffer.getSize());
    Serial.printf("Buffer is in %s\n", g_psramBuffer.isPSRAM() ? "PSRAM" : "regular memory");
  }

  File file = fs.open(path);
  if (!file)
  {
    Serial.println("Failed to open file for reading");
    return;
  }

  uint8_t *buffer = g_psramBuffer.getBuffer();
  size_t bufferSize = g_psramBuffer.getSize();
  size_t bytesRead = 0;
  size_t totalBytesRead = 0;
  uint32_t startTime = millis();

  Serial.print("Read from file: ");
  while ((bytesRead = file.read(buffer, bufferSize)) > 0)
  {
    totalBytesRead += bytesRead;
    // For demonstration, we'll just print the first 100 bytes
    if (totalBytesRead <= 100)
    {
      for (size_t i = 0; i < min(bytesRead, (size_t)100); i++)
      {
        Serial.write(buffer[i]);
      }
    }
  }

  uint32_t endTime = millis();
  Serial.printf("\nRead %u bytes in %u ms (%.2f KB/s)\n",
                totalBytesRead,
                endTime - startTime,
                totalBytesRead / (float)(endTime - startTime));

  file.close();
}

void writeFile_PSRAM(fs::FS &fs, const char *path, const char *message)
{
  Serial.printf("Writing file with PSRAM buffer: %s\n", path);

  // Initialize PSRAM buffer if not already done
  if (!g_psramBuffer.isInitialized())
  {
    if (!g_psramBuffer.init())
    {
      Serial.println("Failed to initialize PSRAM buffer, falling back to standard function");
      writeFile(fs, path, message);
      return;
    }
    Serial.printf("PSRAM buffer initialized: %u bytes\n", g_psramBuffer.getSize());
    Serial.printf("Buffer is in %s\n", g_psramBuffer.isPSRAM() ? "PSRAM" : "regular memory");
  }

  File file = fs.open(path, FILE_WRITE);
  if (!file)
  {
    Serial.println("Failed to open file for writing");
    return;
  }

  size_t messageLen = strlen(message);
  uint8_t *buffer = g_psramBuffer.getBuffer();
  size_t bufferSize = g_psramBuffer.getSize();

  // Copy message to PSRAM buffer
  size_t bytesToWrite = min(messageLen, bufferSize);
  memcpy(buffer, message, bytesToWrite);

  uint32_t startTime = millis();
  bool success = file.write(buffer, bytesToWrite) == bytesToWrite;
  uint32_t endTime = millis();

  if (success)
  {
    Serial.printf("File written: %u bytes in %u ms (%.2f KB/s)\n",
                  bytesToWrite,
                  endTime - startTime,
                  bytesToWrite / (float)(endTime - startTime));
  }
  else
  {
    Serial.println("Write failed");
  }

  file.close();
}

void appendFile_PSRAM(fs::FS &fs, const char *path, const char *message)
{
  Serial.printf("Appending to file with PSRAM buffer: %s\n", path);

  // Initialize PSRAM buffer if not already done
  if (!g_psramBuffer.isInitialized())
  {
    if (!g_psramBuffer.init())
    {
      Serial.println("Failed to initialize PSRAM buffer, falling back to standard function");
      appendFile(fs, path, message);
      return;
    }
    Serial.printf("PSRAM buffer initialized: %u bytes\n", g_psramBuffer.getSize());
    Serial.printf("Buffer is in %s\n", g_psramBuffer.isPSRAM() ? "PSRAM" : "regular memory");
  }

  File file = fs.open(path, FILE_APPEND);
  if (!file)
  {
    Serial.println("Failed to open file for appending");
    return;
  }

  size_t messageLen = strlen(message);
  uint8_t *buffer = g_psramBuffer.getBuffer();
  size_t bufferSize = g_psramBuffer.getSize();

  // Copy message to PSRAM buffer
  size_t bytesToWrite = min(messageLen, bufferSize);
  memcpy(buffer, message, bytesToWrite);

  uint32_t startTime = millis();
  bool success = file.write(buffer, bytesToWrite) == bytesToWrite;
  uint32_t endTime = millis();

  if (success)
  {
    Serial.printf("Message appended: %u bytes in %u ms (%.2f KB/s)\n",
                  bytesToWrite,
                  endTime - startTime,
                  bytesToWrite / (float)(endTime - startTime));
  }
  else
  {
    Serial.println("Append failed");
  }

  file.close();
}

void testFileIO_PSRAM(fs::FS &fs, const char *path)
{
  // 重置看门狗计时器
  esp_task_wdt_reset();

  // Initialize PSRAM buffer if not already done
  if (!g_psramBuffer.isInitialized())
  {
    if (!g_psramBuffer.init())
    {
      Serial.println("Failed to initialize PSRAM buffer, falling back to standard function");
      testFileIO(fs, path);
      return;
    }
    Serial.printf("PSRAM buffer initialized: %u bytes (%.2f MB)\n",
                  g_psramBuffer.getSize(), g_psramBuffer.getSize() / (1024.0 * 1024.0));
    Serial.printf("Buffer is in %s\n", g_psramBuffer.isPSRAM() ? "PSRAM" : "regular memory");
  }

  uint8_t *buffer = g_psramBuffer.getBuffer();
  size_t bufferSize = g_psramBuffer.getSize();

  // 对于大型缓冲区，限制每次操作的大小以避免看门狗超时
  size_t operationSize = min(bufferSize, (size_t)(256 * 1024)); // 最大256KB的操作块

  Serial.printf("Using operation size of %u bytes (%.2f KB)\n",
                operationSize, operationSize / 1024.0);

  // Read test
  File file = fs.open(path);
  size_t len = 0;
  uint32_t start = millis();
  uint32_t end = start;
  uint32_t lastWdtReset = start;

  if (file)
  {
    len = file.size();
    size_t flen = len;
    start = millis();
    lastWdtReset = start;

    Serial.printf("Starting read test with file size: %u bytes (%.2f MB)\n",
                  flen, flen / (1024.0 * 1024.0));

    while (len)
    {
      // 每秒重置一次看门狗计时器
      uint32_t now = millis();
      if (now - lastWdtReset > 1000)
      {
        esp_task_wdt_reset();
        lastWdtReset = now;
        Serial.print("."); // 显示进度
      }

      size_t toRead = len;
      if (toRead > operationSize)
      {
        toRead = operationSize;
      }

      file.read(buffer, toRead);
      len -= toRead;
      yield(); // 让出CPU，防止WDT超时
    }

    end = millis() - start;
    Serial.printf("\n%u bytes read for %u ms (%.2f KB/s)\r\n",
                  flen, end, flen / (float)end);
    file.close();
  }
  else
  {
    Serial.println("Failed to open file for reading");
  }

  // 重置看门狗计时器
  esp_task_wdt_reset();

  // Write test - 使用较小的测试文件以避免超时
  size_t testSize = 1 * 1024 * 1024; // 使用1MB而不是2MB
  Serial.printf("Starting write test with size: %u bytes (%.2f MB)\n",
                testSize, testSize / (1024.0 * 1024.0));

  file = fs.open(path, FILE_WRITE);
  if (!file)
  {
    Serial.println("Failed to open file for writing");
    return;
  }

  // Fill buffer with random data - 只填充我们将使用的部分
  for (size_t i = 0; i < operationSize; i++)
  {
    buffer[i] = random(0, 255);

    // 每32KB重置一次看门狗
    if (i % (32 * 1024) == 0)
    {
      yield();
    }
  }

  // 重置看门狗计时器
  esp_task_wdt_reset();

  start = millis();
  lastWdtReset = start;

  // Write data in smaller chunks to avoid WDT timeout
  size_t bytesWritten = 0;
  size_t chunkSize = min(operationSize, (size_t)(64 * 1024)); // 最大64KB的块

  while (bytesWritten < testSize)
  {
    // 每秒重置一次看门狗计时器
    uint32_t now = millis();
    if (now - lastWdtReset > 1000)
    {
      esp_task_wdt_reset();
      lastWdtReset = now;
      Serial.print("."); // 显示进度
    }

    size_t toWrite = min(chunkSize, testSize - bytesWritten);
    file.write(buffer, toWrite);
    bytesWritten += toWrite;
    yield(); // 让出CPU，防止WDT超时
  }

  end = millis() - start;
  Serial.printf("\n%u bytes written for %u ms (%.2f KB/s)\n",
                bytesWritten, end, bytesWritten / (float)end);
  file.close();

  // 最后再次重置看门狗计时器
  esp_task_wdt_reset();
}