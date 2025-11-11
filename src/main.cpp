#include <Arduino.h>
#include <TFT_eSPI.h> // Graphics library
#include <SPI.h>
#include <XPT2046_Touchscreen.h> // Touch library
#include <SPIFFS.h>
#include "luaDriver.hpp"
#include "flags.h"

#if ENABLE_BLE
#include "simpleBle.hpp"
#endif

// --- Configuration ---
// TFT_eSPI is configured via build_flags in platformio.ini

// XPT2046 Touch Controller setup
#ifndef TOUCH_CS
#define TOUCH_CS 33 // fallback default
#endif
#ifndef TOUCH_IRQ
#define TOUCH_IRQ 36 // fallback default
#endif
#define CS_PIN TOUCH_CS
#define T_IRQ_PIN TOUCH_IRQ

#ifdef USE_VSPI
#define TOUCH_SCLK 25
#define TOUCH_MISO 39
#define TOUCH_MOSI 32
SPIClass touchSPI(VSPI); // Use VSPI for the touch controller
#else
SPIClass touchSPI(HSPI); // Use HSPI for the touch controller
#endif

// LEDs are 4, 16, 17 for CYD (R, G, B)
XPT2046_Touchscreen ts(CS_PIN, T_IRQ_PIN);
TFT_eSPI tft = TFT_eSPI();

// --- Function Declarations ---
void runDiagnostics(const char *message);

LuaDriver luaDriver(&tft, &ts);

#if ENABLE_BLE
SimpleBle simpleBle;
#endif

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
  tft.fillScreen(TFT_BLACK);
  tft.initDMA();

#ifdef USE_VSPI
  touchSPI.begin(TOUCH_SCLK, TOUCH_MISO, TOUCH_MOSI, -1);
#else
  touchSPI.begin(TFT_SCLK, TFT_MISO, TFT_MOSI, -1);
#endif
  ts.begin(touchSPI);

  ts.setRotation(1);

  luaDriver.begin();

#if LUA_FROM_FILE
  if (!SPIFFS.begin(false))
  {
    Serial.println("FS mount failed");
  }
  else
  {
    Serial.println("FS mounted successfully");
  }

  luaDriver.loadLuaFromFS();
#endif

#if ENABLE_BLE
  if (simpleBle.begin())
  {
    luaDriver.setIsKeyDownCallback(simpleBle.isKeyDown);
  }
#endif

  runDiagnostics("Lua driver initialized");

#if LUA_FROM_FILE
  SPIFFS.end();
#endif
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

  Serial.printf("TFT init: w=%d h=%d rot=%d\n", tft.width(), tft.height(), tft.getRotation());

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

#if LUA_FROM_FILE
  // FS diagnostics
  size_t total = SPIFFS.totalBytes();
  size_t used = SPIFFS.usedBytes();
  Serial.printf("FS: %u used / %u total (%.1f%%)\n",
                used, total, 100.0 * used / total);

  File root = SPIFFS.open("/");
  File file = root.openNextFile();
  while (file)
  {
    Serial.printf("FILE: /%s (%d bytes)\n", file.name(), file.size());
    file = root.openNextFile();
  }
#endif
}

void loop()
{
  luaDriver.loop();
}
