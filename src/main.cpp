#include <Arduino.h>
#include <TFT_eSPI.h> // Graphics library
#include <SPI.h>
#include <XPT2046_Touchscreen.h> // Touch library
#include "luaDriver.hpp"
#include "loopExample.hpp"

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

// LEDs are 4, 16, 17 for CYD (R, G, B)

// Note: The XPT2046 shares the hardware SPI bus with the TFT.
// Construct the touch object with CS/TIRQ only and call begin() with the SPI instance
XPT2046_Touchscreen ts(CS_PIN, T_IRQ_PIN);

// --- Game / example objects ---
TFT_eSPI tft = TFT_eSPI();

#define TS_MIN_X 240
#define TS_MAX_X 3780
#define TS_MIN_Y 220
#define TS_MAX_Y 3800

// --- Function Declarations ---
void runDiagnostics(const char *message);

LoopExample loopExample(&tft, &ts);

LuaDriver luaDriver(&tft, &ts);

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
  tft.initDMA();

  // Initialize example
  loopExample.begin();

#ifdef USE_VSPI
  touchSPI.begin(TOUCH_SCLK, TOUCH_MISO, TOUCH_MOSI, -1);
#else
  touchSPI.begin(TFT_SCLK, TFT_MISO, TFT_MOSI, -1);
#endif
  ts.begin(touchSPI);

  ts.setRotation(1);

  runDiagnostics("Initial setup complete");

  luaDriver.begin();

  runDiagnostics("Lua driver initialized");
}

void runDiagnostics(const char *message = nullptr)
{
  if (message)
  {
    Serial.println("================================");
    Serial.println(message);
    Serial.println("================================");
  }

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

void loop()
{
  // loopExample.loop();
  luaDriver.loop();
}
