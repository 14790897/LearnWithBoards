#include <Arduino.h>
#include <SPI.h>
#include <Ucglib.h>
#include <avr/wdt.h>

// 初始化 ST7735 显示屏 (128x160, 硬件 SPI)
Ucglib_ST7735_18x128x160_HWSPI ucg(9, 10, 8); //  dc, cs, reset

String inputText = R"(Freenove Freenove Freenove Freenove Freenove Freenove Freenove Freenove Freenove Freenove Freenove Freenove Freenove Freenove Freenove Freenove Freenove 

)";                            // 示例文字
#define SCREEN_WIDTH 130       // 屏幕宽度
#define CHAR_WIDTH 10     // 每个字符的宽度（根据具体字体调整）
#define SCREEN_HEIGHT 90
#define LINE_HEIGHT 16                          // 每行的高度（根据具体字体调整）
#define MAX_LINES (SCREEN_HEIGHT / LINE_HEIGHT) // 每屏最多显示的行数

unsigned long lastUpdateTime = 0;          // 上一次更新时间
const unsigned long updateInterval = 5000; // 每隔 5 秒刷新屏幕
int currentPage = 0;                       // 当前显示的页数
int totalPages = 0;                        // 总页数
String pages[20];                          // 存储分页后的文字内容

// 将输入文字分页
void paginateText(String text)
{
    int charsPerLine = SCREEN_WIDTH / CHAR_WIDTH; // 每行可容纳的字符数
    int currentCharIndex = 0;                     // 当前字符索引
    String currentPageText = "";                  // 当前页的文字内容

    while (currentCharIndex < text.length())
    {
        String currentLine = text.substring(currentCharIndex, currentCharIndex + charsPerLine);
        currentCharIndex += charsPerLine;

        // 如果当前页已满（达到最大行数），保存当前页并开始新页
        if (currentPageText.length() / charsPerLine >= MAX_LINES)
        {
            pages[totalPages++] = currentPageText;
            currentPageText = "";
        }

        currentPageText += currentLine + "\n"; // 添加当前行并换行
    }

    // 保存最后一页
    if (currentPageText.length() > 0)
    {
        pages[totalPages++] = currentPageText;
    }

    Serial.println("分页完成，总页数: " + String(totalPages));
    for (int i = 0; i < totalPages; i++)
    {
        Serial.println("第 " + String(i + 1) + " 页内容:");
        Serial.println(pages[i]);
    }
}

// 显示指定页
void displayPage(int page)
{
    if (page >= totalPages)
        return;

    ucg.clearScreen();           // 清屏
    ucg.setColor(255, 255, 255); // 设置字体颜色为白色

    int y = LINE_HEIGHT; // 起始行高度
    String pageContent = pages[page];
    int startIndex = 0;

    // 按行显示文字
    while (startIndex < pageContent.length())
    {
        int endIndex = pageContent.indexOf('\n', startIndex);
        if (endIndex == -1)
            endIndex = pageContent.length();

        String line = pageContent.substring(startIndex, endIndex);
        ucg.setPrintPos(0, y);
        ucg.print(line);

        y += LINE_HEIGHT;
        startIndex = endIndex + 1;
    }
}

void setup()
{
    Serial.begin(115200);
    wdt_enable(WDTO_2S); // 启用看门狗，超时时间为 2 秒

    Serial.println("初始化显示屏...");
    // 初始化显示屏
    ucg.begin(UCG_FONT_MODE_TRANSPARENT);
    ucg.setRotate90(); // 旋转 90 度，变为 160x128
    ucg.clearScreen();
    ucg.setFont(ucg_font_ncenR14_tr); // 设置字体

    // 分割文字
    paginateText(inputText);

    // 显示第一页
    displayPage(currentPage);
}

void loop()
{
    wdt_reset(); // 重置看门狗计时器

    // 检查是否到了刷新时间
    if (millis() - lastUpdateTime >= updateInterval)
    {
        lastUpdateTime = millis();                    // 更新刷新时间
        currentPage = (currentPage + 1) % totalPages; // 切换到下一页（循环显示）
        displayPage(currentPage);                     // 显示当前页
    }
}
