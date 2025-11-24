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

#if ENABLE_WIFI
#ifndef WIFI_SSID
#define WIFI_SSID "UnknownSSID"
#endif
#ifndef WIFI_PASSWORD
#define WIFI_PASSWORD "UnknownPassword"
#endif
#define WIFI_TIMEOUT 15000 // 15 seconds
#include <WiFi.h>

void initializeWiFi()
{
  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.println("WiFi already connected");
    return;
  }

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.print("Connecting to WiFi");

  unsigned long startTime = millis();
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");

    if (millis() - startTime > WIFI_TIMEOUT)
    {
      Serial.println("\nWiFi connection timeout!");
      Serial.println("Please check your credentials");
      break;
    }
  }

  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.println("\nWiFi connected!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  }
}
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

#ifdef USE_VSPI
  touchSPI.begin(TOUCH_SCLK, TOUCH_MISO, TOUCH_MOSI, -1);
#else
  touchSPI.begin(TFT_SCLK, TFT_MISO, TFT_MOSI, -1);
#endif
  ts.begin(touchSPI);

  ts.setRotation(1);

  luaDriver.begin();

#if ENABLE_BLE
  if (simpleBle.begin())
  {
    luaDriver.setIsKeyDownCallback(simpleBle.isKeyDown);
  }
#endif

#if ENABLE_WIFI
  // Register WiFi initialization callback - will be called on-demand by ws_connect
  // to save memory by avoiding the initialization WiFi if not used by the Lua script
  luaDriver.setWiFiInitCallback(initializeWiFi);
#endif

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

  Serial.println("================================\n");
  Serial.printf("WiFi parameters:\nSSID: %s\nPassword: %s\n", WIFI_SSID, WIFI_PASSWORD);
  Serial.println("================================\n");
}

void loop()
{
  luaDriver.loop();
}
