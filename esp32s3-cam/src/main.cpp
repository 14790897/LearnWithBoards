#include <Arduino.h>
#include "driver/sdmmc_host.h" // SDMMC 主机驱动
#include "esp_vfs_fat.h"       // FAT 文件系统
#include "sdmmc_cmd.h"         // SDMMC 命令

// SD 卡引脚定义（根据你的 SD.h）
#define BSP_SD_CLK GPIO_NUM_47 // 时钟引脚
#define BSP_SD_CMD GPIO_NUM_48 // 命令引脚
#define BSP_SD_D0 GPIO_NUM_21  // 数据0引脚
#define MOUNT_POINT "/sdcard"  // 挂载点

void setup()
{
    Serial.begin(115200);
    delay(1000);

    // SDMMC 配置
    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
    slot_config.width = 1;        // 1-bit 模式
    slot_config.clk = BSP_SD_CLK; // 时钟引脚
    slot_config.cmd = BSP_SD_CMD; // 命令引脚
    slot_config.d0 = BSP_SD_D0;   // 数据0引脚

    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = true, // 挂载失败时格式化
        .max_files = 5                  // 最大打开文件数
    };

    sdmmc_card_t *card;
    // 挂载 SD 卡
    esp_err_t ret = esp_vfs_fat_sdmmc_mount(MOUNT_POINT, &host, &slot_config, &mount_config, &card);
    if (ret != ESP_OK)
    {
        Serial.println("Failed to mount SD card");
        return;
    }
    Serial.println("SD card mounted");

    // 写文件
    FILE *f = fopen(MOUNT_POINT "/hello.txt", "w");
    if (f == NULL)
    {
        Serial.println("Failed to open file for writing");
        return;
    }
    fprintf(f, "Hello SD Card!\n");
    fclose(f);
    Serial.println("File written");

    // 读文件
    f = fopen(MOUNT_POINT "/hello.txt", "r");
    if (f == NULL)
    {
        Serial.println("Failed to open file for reading");
        return;
    }
    char line[64];
    fgets(line, sizeof(line), f);
    fclose(f);
    Serial.print("Read from file: ");
    Serial.println(line);

    // 卸载 SD 卡
    esp_vfs_fat_sdcard_unmount(MOUNT_POINT, card);
    Serial.println("SD card unmounted");
}

void loop()
{
}