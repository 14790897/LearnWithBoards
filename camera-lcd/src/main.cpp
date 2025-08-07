/*
 * LCD Test Program for Arduino
 * Tests ST7735S 128x160 SPI LCD Display
 *
 * Hardware Connections:
 * ST7735S LCD -> ESP32-S3
 * VCC -> 3.3V
 * GND -> GND
 * CS  -> Pin 15
 * RESET -> Pin 42
 * DC  -> Pin 48
 * SDI(MOSI) -> Pin 2
 * SCK -> Pin 1
 * LED -> Pin 16 (Backlight)
 * SDO(MISO) -> Pin 17 (optional)
 */

#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>

// Pin definitions (matching original ESP-IDF configuration)
#define TFT_CS 15   // Chip select
#define TFT_RST 42  // Reset
#define TFT_DC 48   // Data/Command
#define TFT_MOSI 2  // SPI MOSI
#define TFT_SCLK 1  // SPI Clock
#define TFT_MISO 17 // SPI MISO (optional for display)
#define TFT_BL 16   // Backlight

// LCD dimensions (ST7735S is typically 128x160, but can vary)
#define LCD_WIDTH 128
#define LCD_HEIGHT 160

// Create display object for ST7735S with custom SPI pins
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);

// RGB565 color definitions
#define COLOR_BLACK 0x0000
#define COLOR_WHITE 0xFFFF
#define COLOR_RED 0xF800
#define COLOR_GREEN 0x07E0
#define COLOR_BLUE 0x001F
#define COLOR_YELLOW 0xFFE0
#define COLOR_CYAN 0x07FF
#define COLOR_MAGENTA 0xF81F
// Function declarations
void runLCDTests();
void animateColorBars();
void drawColorBars();
void testTextDisplay();
void testShapes();

void setup()
{
  // Initialize Serial
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("=== ESP32-S3 ST7735S LCD Test ===");
  
  // Initialize pins
  pinMode(TFT_BL, OUTPUT);
  pinMode(TFT_RST, OUTPUT);
  pinMode(TFT_CS, OUTPUT);
  pinMode(TFT_DC, OUTPUT);
  
  // Turn on backlight
  digitalWrite(TFT_BL, HIGH);

  // Initialize ST7735S display
  tft.initR(INITR_BLACKTAB);
  tft.setRotation(1); // Landscape mode

  Serial.println("LCD initialized successfully!");

  // Run LCD tests
  runLCDTests();
}

void loop()
{
  // Keep running color pattern animation
  animateColorBars();
  delay(2000);
}

void runLCDTests()
{
  // Test 1: Basic colors
  tft.fillScreen(ST77XX_RED);
  delay(1000);
  tft.fillScreen(ST77XX_GREEN);
  delay(1000);
  tft.fillScreen(ST77XX_BLUE);
  delay(1000);
  tft.fillScreen(ST77XX_WHITE);
  delay(1000);
  tft.fillScreen(ST77XX_BLACK);
  delay(1000);

  // Test 2: Color bars
  drawColorBars();
  delay(3000);

  // Test 3: Text display
  testTextDisplay();
  delay(3000);

  // Test 4: Geometric shapes
  testShapes();
  delay(3000);

  Serial.println("All tests completed!");
}

void drawColorBars()
{
  // For 128x160 display in landscape (160x128)
  int barWidth = 160 / 7;  // 7 color bars
  uint16_t colors[] = {
    ST77XX_RED, ST77XX_YELLOW, ST77XX_GREEN, 
    ST77XX_CYAN, ST77XX_BLUE, ST77XX_MAGENTA, ST77XX_WHITE
  };

  for (int i = 0; i < 7; i++)
  {
    int x = i * barWidth;
    int width = (i == 6) ? 160 - x : barWidth; // Last bar fills remaining space
    tft.fillRect(x, 0, width, 128, colors[i]);
  }
}

void testTextDisplay()
{
  tft.fillScreen(ST77XX_BLACK);

  // Test different text sizes and colors
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(1);
  tft.setCursor(5, 5);
  tft.println("ST7735S Test");

  tft.setTextColor(ST77XX_RED);
  tft.setTextSize(1);
  tft.setCursor(5, 20);
  tft.println("128x160 Display");

  tft.setTextColor(ST77XX_GREEN);
  tft.setTextSize(2);
  tft.setCursor(5, 35);
  tft.println("Size 2");

  tft.setTextColor(ST77XX_BLUE);
  tft.setTextSize(1);
  tft.setCursor(5, 60);
  tft.println("ESP32-S3");

  tft.setTextColor(ST77XX_YELLOW);
  tft.setCursor(5, 75);
  tft.println("Arduino");

  tft.setTextColor(ST77XX_CYAN);
  tft.setCursor(5, 90);
  tft.println("Working!");

  tft.setTextColor(ST77XX_MAGENTA);
  tft.setCursor(5, 105);
  tft.println("Ready for");
  tft.setCursor(5, 115);
  tft.println("Camera!");
}

void testShapes()
{
  tft.fillScreen(ST77XX_BLACK);

  // Draw rectangles
  tft.drawRect(5, 5, 30, 20, ST77XX_RED);
  tft.fillRect(40, 5, 30, 20, ST77XX_GREEN);

  // Draw circles
  tft.drawCircle(25, 50, 15, ST77XX_BLUE);
  tft.fillCircle(55, 50, 15, ST77XX_YELLOW);

  // Draw triangles
  tft.drawTriangle(100, 30, 85, 60, 115, 60, ST77XX_CYAN);
  tft.fillTriangle(130, 30, 115, 60, 145, 60, ST77XX_MAGENTA);

  // Draw lines
  for (int i = 0; i < 8; i++)
  {
    tft.drawLine(5 + i * 3, 80, 5 + i * 3, 120, ST77XX_WHITE);
  }

  // Draw rounded rectangles
  tft.drawRoundRect(80, 80, 40, 25, 5, ST77XX_RED);
  tft.fillRoundRect(125, 80, 30, 25, 3, ST77XX_GREEN);
}

void animateColorBars()
{
  static int offset = 0;
  int barWidth = 160 / 7;
  uint16_t colors[] = {
    ST77XX_RED, ST77XX_YELLOW, ST77XX_GREEN, 
    ST77XX_CYAN, ST77XX_BLUE, ST77XX_MAGENTA, ST77XX_WHITE
  };

  for (int i = 0; i < 7; i++)
  {
    int colorIndex = (i + offset) % 7;
    int x = i * barWidth;
    int width = (i == 6) ? 160 - x : barWidth;
    tft.fillRect(x, 0, width, 128, colors[colorIndex]);
  }

  offset = (offset + 1) % 7;
}
