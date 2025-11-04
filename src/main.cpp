#include <Arduino.h>
#include <TFT_eSPI.h> // Graphics library
#include <SPI.h>
#include <XPT2046_Touchscreen.h> // Touch library

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

#if defined(TFT_BL)
  Serial.printf("TFT_BL pin=%d state=%d\n", TFT_BL, digitalRead(TFT_BL));
#endif

  tft.setRotation(2);
  tft.fillScreen(BACKGROUND_COLOR);

  // Now that rotation is set, center the ball on the visible screen
  ballX = tft.width() / 2.0f;
  ballY = tft.height() / 2.0f;

  // Print diagnostics so user can verify display/backlight state over Serial
  Serial.printf("TFT init: w=%d h=%d rot=%d\n", tft.width(), tft.height(), tft.getRotation());
#if defined(TFT_BL)
  Serial.printf("TFT_BL pin=%d state=%d\n", TFT_BL, digitalRead(TFT_BL));
#endif

#ifdef USE_VSPI
  touchSPI.begin(TOUCH_SCLK, TOUCH_MISO, TOUCH_MOSI, -1);
#else
  touchSPI.begin(TFT_SCLK, TFT_MISO, TFT_MOSI, -1);
#endif
  ts.begin(touchSPI);

  ts.setRotation(1); // Match rotation of the display
}

// --- Loop ---
void loop()
{
  // 1. Handle Input
  handleTouch();

  // 2. Erase (Draw the ball in the background color at the old position)
  drawBall(BACKGROUND_COLOR, previousColor);

  // 3. Update Physics
  updateBallPosition();

  // 4. Draw (Draw the ball in the current color at the new position)
  drawBall(ballColor, BACKGROUND_COLOR);

  // Save the current color for the next frame's erase step
  previousColor = ballColor;

  // Frame rate control (optional, but helps avoid flicker/excessive speed)
  delay(40); // (1000ms / 40ms = 25 FPS)
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
  // Update position based on velocity
  ballX += velocityX;
  ballY += velocityY;

  // Get screen boundaries
  int minX = (int)ballRadius;
  int maxX = tft.width() - (int)ballRadius;
  int minY = (int)ballRadius;
  int maxY = tft.height() - (int)ballRadius;

  // Serial.printf("ball: (%g, %g)\n", ballX, ballY);

  // Check for horizontal boundary collision
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
    // Read the touch point raw data
    TS_Point p = ts.getPoint();

    // Map the raw touch data to the actual screen pixel coordinates
    // Note: X and Y might be swapped or inverted depending on your board's wiring and rotation
    int pixelX = map(p.x, TS_MIN_X, TS_MAX_X, 0, tft.width());
    int pixelY = map(p.y, TS_MIN_Y, TS_MAX_Y, 0, tft.height());

    // Serial.printf("Touch at raw(%d,%d) mapped(%d,%d) z=%d\n", p.x, p.y, pixelX, pixelY, p.z);

    // Check if the touch is near the ball (optional: allows changing color only when touching the ball)
    float distance = sqrt(pow(pixelX - ballX, 2) + pow(pixelY - ballY, 2));

    if (distance <= ballRadius * 2)
    {
      // Change color on touch
      // Cycle through a few colors
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
