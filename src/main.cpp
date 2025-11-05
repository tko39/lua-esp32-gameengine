#include <Arduino.h>
#include <TFT_eSPI.h> // Graphics library
#include <SPI.h>
#include <XPT2046_Touchscreen.h> // Touch library
#include "lua_driver.hpp"

// --- Configuration ---
// TFT_eSPI is configured via build_flags in platformio.ini

// XPT2046 Touch Controller setup
#ifndef TOUCH_CS
#define TOUCH_CS 33 // fallback default, update if needed
#endif
#ifndef TOUCH_IRQ
#define TOUCH_IRQ 36 // fallback default, update if needed
#endif
#define CS_PIN TOUCH_CS
#define T_IRQ_PIN TOUCH_IRQ

#define DO_LOOP 1

#define USE_VSPI

#define BACKGROUND_COLOR TFT_BLACK
#ifdef USE_VSPI
#define TOUCH_SCLK 25
#define TOUCH_MISO 39
#define TOUCH_MOSI 32
SPIClass touchSPI(VSPI); // Use VSPI for the touch controller
#else
SPIClass touchSPI(HSPI); // Use HSPI for the touch controller
#endif

// Note: The XPT2046 shares the hardware SPI bus with the TFT.
// Construct the touch object with CS/TIRQ only and call begin() with the SPI instance
XPT2046_Touchscreen ts(CS_PIN, T_IRQ_PIN);

// --- Game Variables ---
TFT_eSPI tft = TFT_eSPI();

// Ball properties
float ballX = 120.0;
float ballY = 160.0;
float ballRadius = 20.0;
float velocityX = 4.0;
float velocityY = 3.5;

uint16_t ballColor = TFT_WHITE;
uint16_t previousColor = BACKGROUND_COLOR; // Used for "erasing"

// Touch mapping (adjust these constants based on calibration if needed)
// These values are often suitable for ILI9341
#define TS_MIN_X 240
#define TS_MAX_X 3780
#define TS_MIN_Y 220
#define TS_MAX_Y 3800

// --- Function Declarations ---
void updateBallPosition();
void drawBall(uint16_t color, uint16_t eraseColor);
void handleTouch();
void runDiagnostics();

// --- Setup ---
void setup()
{
  Serial.begin(115200);

  tft.init();
#if defined(TFT_BL)
  // Ensure backlight pin is driven on (some boards require explicit control)
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, TFT_BACKLIGHT_ON);
#endif

  tft.setRotation(2);
  tft.fillScreen(BACKGROUND_COLOR);

  // Now that rotation is set, center the ball on the visible screen
  ballX = tft.width() / 2.0f;
  ballY = tft.height() / 2.0f;

#ifdef USE_VSPI
  touchSPI.begin(TOUCH_SCLK, TOUCH_MISO, TOUCH_MOSI, -1);
#else
  touchSPI.begin(TFT_SCLK, TFT_MISO, TFT_MOSI, -1);
#endif
  ts.begin(touchSPI);

  ts.setRotation(1);

  runDiagnostics();

  setup_lua();
}

void runDiagnostics()
{
#if defined(TFT_BL)
  Serial.printf("TFT_BL pin=%d state=%d\n", TFT_BL, digitalRead(TFT_BL));
#endif

  // Print diagnostics so user can verify display/backlight state over Serial
  Serial.printf("TFT init: w=%d h=%d rot=%d\n", tft.width(), tft.height(), tft.getRotation());
#if defined(TFT_BL)
  Serial.printf("TFT_BL pin=%d state=%d\n", TFT_BL, digitalRead(TFT_BL));
#endif

  if (psramFound())
  {
    Serial.println("\nPSRAM found and enabled!");
    Serial.printf("Total PSRAM: %d bytes\n", ESP.getPsramSize());
    Serial.printf("Free PSRAM: %d bytes\n", ESP.getFreePsram());
  }
  else
  {
    Serial.println("PSRAM not found!");
  }

  Serial.printf("Total Internal SRAM: %d bytes\n", ESP.getHeapSize());
  Serial.printf("Free Internal SRAM: %d bytes\n", ESP.getFreeHeap());

  // Total flash size (bytes)
  Serial.printf("Flash size: %u bytes\n", ESP.getFlashChipSize());
  Serial.printf("Flash speed: %u Hz\n", ESP.getFlashChipSpeed());
  Serial.printf("Flash mode: %s\n",
                ESP.getFlashChipMode() == FM_QIO ? "QIO" : ESP.getFlashChipMode() == FM_QOUT ? "QOUT"
                                                       : ESP.getFlashChipMode() == FM_DIO    ? "DIO"
                                                       : ESP.getFlashChipMode() == FM_DOUT   ? "DOUT"
                                                                                             : "UNKNOWN");
}

void loopBall()
{
  handleTouch();
  drawBall(BACKGROUND_COLOR, previousColor);
  updateBallPosition();
  drawBall(ballColor, BACKGROUND_COLOR);
  previousColor = ballColor;
  delay(40);
}

// --- Loop ---
void loop()
{
  // loopBall();
  loop_lua();
}

// --- Drawing Function ---
void drawBall(uint16_t color, uint16_t eraseColor)
{
  // Only redraw if the color has changed since the last draw at this location
  // or if we are erasing (color = BACKGROUND_COLOR)
  if (color != eraseColor)
  {
    // This uses the fillCircle function, which is fast in TFT_eSPI
    tft.fillCircle((int)ballX, (int)ballY, (int)ballRadius, color);
  }
}

// --- Physics Update Function ---
void updateBallPosition()
{
  ballX += velocityX;
  ballY += velocityY;

  int minX = (int)ballRadius;
  int maxX = tft.width() - (int)ballRadius;
  int minY = (int)ballRadius;
  int maxY = tft.height() - (int)ballRadius;

  if (ballX < minX)
  {
    ballX = minX;
    velocityX = -velocityX; // Reverse direction
  }
  else if (ballX > maxX)
  {
    ballX = maxX;
    velocityX = -velocityX; // Reverse direction
  }

  // Check for vertical boundary collision
  if (ballY < minY)
  {
    ballY = minY;
    velocityY = -velocityY; // Reverse direction
  }
  else if (ballY > maxY)
  {
    ballY = maxY;
    velocityY = -velocityY; // Reverse direction
  }
}

// --- Touch Handling Function ---
void handleTouch()
{
  if (ts.tirqTouched() && ts.touched())
  {
    TS_Point p = ts.getPoint();

    int pixelX = map(p.x, TS_MIN_X, TS_MAX_X, 0, tft.width());
    int pixelY = map(p.y, TS_MIN_Y, TS_MAX_Y, 0, tft.height());
    float distance = sqrt(pow(pixelX - ballX, 2) + pow(pixelY - ballY, 2));

    if (distance <= ballRadius * 2)
    {
      switch (ballColor)
      {
      case TFT_RED:
        ballColor = TFT_GREEN;
        break;
      case TFT_GREEN:
        ballColor = TFT_BLUE;
        break;
      case TFT_BLUE:
        ballColor = TFT_YELLOW;
        break;
      case TFT_YELLOW:
        ballColor = TFT_MAGENTA;
        break;
      case TFT_MAGENTA:
        ballColor = TFT_CYAN;
        break;
      case TFT_CYAN:
      default:
        ballColor = TFT_RED;
        break;
      }
    }
  }
}

// region: Xterm 256 color mapping utilities
static inline uint8_t quant6(uint8_t v)
{
  // Map 0–255 to 0–5 (round to nearest)
  return (v < 48) ? 0 : (v - 35) / 40; // simple empiric fit
  // or equivalently: min(5, (v + 25) / 51);
}

uint8_t mapToXterm216(uint8_t r, uint8_t g, uint8_t b)
{
  uint8_t R = min(5, (r + 25) / 51);
  uint8_t G = min(5, (g + 25) / 51);
  uint8_t B = min(5, (b + 25) / 51);
  return 16 + 36 * R + 6 * G + B;
}

uint8_t mapToXterm(uint8_t r, uint8_t g, uint8_t b)
{
  // if roughly neutral (within ±10 of each other), use grayscale
  int maxc = max(r, max(g, b));
  int minc = min(r, min(g, b));
  if (maxc - minc < 10)
  {
    // 8 + n*10 (0..23) → 232..255
    uint8_t gray = (r + g + b) / 3;
    uint8_t n = min(23, (gray - 8) / 10);
    return 232 + n;
  }
  return mapToXterm216(r, g, b);
}

static void buildXtermPalette565(uint16_t *xterm565)
{
  static const uint8_t lvl[6] = {0, 95, 135, 175, 215, 255};
  for (int i = 0; i < 256; ++i)
  {
    uint8_t r, g, b;
    if (i < 16)
    { /* handle system colors if you want */
    }
    else if (i < 232)
    {
      int c = i - 16;
      int R = c / 36, G = (c / 6) % 6, B = c % 6;
      r = lvl[R];
      g = lvl[G];
      b = lvl[B];
    }
    else
    {
      uint8_t v = 8 + (i - 232) * 10;
      r = g = b = v;
    }
    xterm565[i] = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
  }
}

/*
void puttingItTogether()
{
  uint16_t xterm565[256];
  // Setup once
  buildXtermPalette565(xterm565);
  for (int i = 0; i < 256; ++i)
    spr.setPaletteColor(i, xterm565[i]);

  // Every frame
  for (int y = 0; y < 240; ++y)
    for (int x = 0; x < 320; ++x)
    {
      uint8_t r, g, b = ...; // your game’s intended color
      uint8_t idx = mapToXterm216(r, g, b);
      spr.drawPixel(x, y, idx); // draws with palette index
    }

  spr.pushSprite(0, 0); // TFT_eSPI converts indices -> RGB565 automatically
}
*/
