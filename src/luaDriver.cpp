#include <Arduino.h>

// Lua is a C library; ensure we include the C headers with C linkage when compiling as C++.
#ifdef __cplusplus
extern "C"
{
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
}
#else
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
#endif

#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include <ArduinoJson.h>
#include <cstdio>
#include <cmath>
#include <algorithm>

#include "luaScript.h"
#include <memory>
#include <SPIFFS.h>
#include "flags.h"
#include "luaDriver.hpp"

// C-style Lua hooks (need C linkage compatible signatures)
static int luaLedControl(lua_State *L);
static int luaAnalogRead(lua_State *L);
static int luaDelayMs(lua_State *L);
static int luaMillis(lua_State *L);
static int luaPrint(lua_State *L);
static int luaGetSystemInfo(lua_State *L);

LuaDriver::LuaDriver(TFT_eSPI *tft, XPT2046_Touchscreen *ts)
    : L_(nullptr), led_status_(0), tft_(tft), ts_(ts), spr_(nullptr), fov3d_(200.0f), camDist3d_(100.0f)
#if ENABLE_WIFI
      ,
      wsCallbackRef_(LUA_NOREF)
#endif
{
}

LuaDriver::~LuaDriver()
{
    if (L_)
    {
        lua_close(L_);
    }
}

void LuaDriver::begin()
{
    // Create sprite with display dimensions and 8-bit color depth
    int w = tft_ ? tft_->width() : 320;
    int h = tft_ ? tft_->height() : 240;

    spr_ = new TFT_eSprite(tft_);
    spr_->setColorDepth(8);
    if (spr_->createSprite(w, h, 1))
    {
        Serial.println("Created sprite successfully");
    }
    else
    {
        Serial.println("Failed to create sprite");
    }

    Serial.println(LUA_RELEASE);

    Serial.println("ESP32 Lua Interpreter Starting...");
    L_ = luaL_newstate();
    if (!L_)
    {
        Serial.println("Failed to create Lua state");
        return;
    }

    luaL_openlibs(L_);
    registerFunctions();

#if DEBUG_SHOW_AVAILABLE_COLORS
    Serial.println("Available colors:");
    Serial.println("{");
    for (int i = 0; i < 256; i++)
    {
        int color16 = tft_->color8to16(i);
        int color24 = tft_->color16to24(color16);

        Serial.printf("%d: { 16: \"%04X\", 24: \"#%06X\" }%c\n", i, color16, color24, (i < 255) ? ',' : ' ');
    }
    Serial.println("}");
#endif
}

void LuaDriver::setIsKeyDownCallback(KeyPressedCallback callback)
{
    isPressedCallback_ = callback;
}

#if ENABLE_WIFI
void LuaDriver::setWiFiInitCallback(WiFiInitCallback callback)
{
    wifiInitCallback_ = callback;
}
#endif

void LuaDriver::loadLuaFromFS()
{
#if LUA_FROM_FILE
    if (!this->cached_lua_fs_buf && !this->cached_lua_fs_sz)
    {
        File f = SPIFFS.open(RUNTIME_LUA_FILE, "r");
        if (!f)
        {
            Serial.printf("cannot open %s\n", RUNTIME_LUA_FILE);
        }

        size_t sz = f.size();
        if (sz == 0)
        {
            Serial.printf("file %s is empty\n", RUNTIME_LUA_FILE);
        }
        else
        {
            std::unique_ptr<char[]> buf(new char[sz + 1]);
            if (f.readBytes(buf.get(), sz) != sz)
            {
                Serial.println("short read");
            }
            else
            {
                buf.get()[sz] = '\0';
                cached_lua_fs_buf = buf.release();
                cached_lua_fs_sz = sz;
                f.close();
            }
        }
    }
#endif
}

int LuaDriver::runLuaFromFS()
{
    // mode=nullptr allows both text and bytecode
    int st = luaL_loadbufferx(L_, cached_lua_fs_buf, cached_lua_fs_sz, RUNTIME_LUA_FILE, nullptr);
    if (st != LUA_OK)
    {
        return st;
    }

    free(cached_lua_fs_buf);
    cached_lua_fs_buf = nullptr;
    cached_lua_fs_sz = 0;

    st = lua_pcall(L_, 0, LUA_MULTRET, 0);
    return st;
}

int LuaDriver::scriptSelectionMenu()
{
    const int numScripts = sizeof(lua_scripts) / sizeof(lua_scripts[0]) - 1;
    const int numScriptNames = sizeof(script_names) / sizeof(script_names[0]) - 1;

    spr_->fillScreen(TFT_BLACK);
    spr_->setTextColor(TFT_WHITE, TFT_BLACK);
    spr_->setTextSize(2);

    spr_->drawString("Select a Lua script to run:\n\n", 10, 10);
    for (int i = 0; i < numScripts; i++)
    {
        if (i >= numScriptNames || script_names[i] == nullptr)
        {
            spr_->drawString(String(i + 1) + ": Script " + String(i + 1) + "\n", 10, 30 + i * 20);
            continue;
        }

        spr_->drawString(String(i + 1) + ": " + String(script_names[i]) + "\n", 10, 30 + i * 20);
    }
    spr_->pushSprite(0, 0);

    while (true)
    {
        if (ts_->touched())
        {
            TS_Point p = ts_->getPoint();
            int y = map(p.y, TS_MIN_Y_CONST, TS_MAX_Y_CONST, 0, tft_->height());
            int index = (y - 30) / 20;
            if (index >= 0 && index < numScripts)
            {
                Serial.printf("Selected script %d\n", index + 1);
                spr_->fillScreen(TFT_BLACK);
                spr_->pushSprite(0, 0);
                return index;
            }
        }
        delay(100);
    }
}

void LuaDriver::loop()
{
#if LUA_FROM_FILE
    const int result = runLuaFromFS();
#else
    const int numScripts = sizeof(lua_scripts) / sizeof(lua_scripts[0]) - 1;
    int scriptIndex = 0;
    switch (numScripts)
    {
    case 0:
    {
        Serial.println("No embedded Lua scripts found!");
        delay(10000);
        return;
    }
    case 1:
    {
        scriptIndex = 0;
        break;
    }
    default:
    {
        Serial.printf("Multiple (%d) embedded Lua scripts found.\n", numScripts);
        scriptIndex = scriptSelectionMenu();
        break;
    }
    }

    const int result = luaL_dostring(L_, lua_scripts[scriptIndex]);
#endif
    if (result != LUA_OK)
    {
        Serial.print("Error in periodic script: ");
        Serial.println(lua_tostring(L_, -1));
        lua_pop(L_, 1);
    }

    Serial.printf("luaScript - finished - restarting in 10 seconds\n");
    delay(10000);
}

lua_State *LuaDriver::state()
{
    return L_;
}

void LuaDriver::callLuaFunctionFromCpp()
{
    lua_getglobal(L_, "lua_callback"); // Get Lua function
    if (lua_isfunction(L_, -1))
    {
        lua_pushinteger(L_, 42); // Push argument
        if (lua_pcall(L_, 1, 0, 0) != LUA_OK)
        {
            Serial.print("Callback error: ");
            Serial.println(lua_tostring(L_, -1));
        }
    }
    lua_pop(L_, 1); // Clean stack
}

void LuaDriver::registerFunctions()
{
    lua_pushcfunction(L_, luaLedControl);
    lua_setglobal(L_, "led");

    lua_pushcfunction(L_, luaAnalogRead);
    lua_setglobal(L_, "analog_read");

    lua_pushcfunction(L_, luaDelayMs);
    lua_setglobal(L_, "delay_ms");

    lua_pushcfunction(L_, luaMillis);
    lua_setglobal(L_, "millis");

    lua_pushcfunction(L_, luaPrint);
    lua_setglobal(L_, "print");

    lua_pushcfunction(L_, luaGetSystemInfo);
    lua_setglobal(L_, "get_system_info");
    // Also register the lge module
    registerLgeModule();
}

// Register a minimal 'lge' table in Lua and attach functions mapping to TFT
void LuaDriver::registerLgeModule()
{
    // Create lge table
    lua_newtable(L_);

    // Push 'this' as upvalue for all lge_* functions
    LuaDriver *self = this;

    // clear_canvas
    lua_pushlightuserdata(L_, self);
    lua_pushcclosure(L_, lge_clear_canvas, 1);
    lua_setfield(L_, -2, "clear_canvas");

    // get_canvas_size
    lua_pushlightuserdata(L_, self);
    lua_pushcclosure(L_, lge_get_canvas_size, 1);
    lua_setfield(L_, -2, "get_canvas_size");

    // draw_circle
    lua_pushlightuserdata(L_, self);
    lua_pushcclosure(L_, lge_draw_circle, 1);
    lua_setfield(L_, -2, "draw_circle");

    // draw_rectangle
    lua_pushlightuserdata(L_, self);
    lua_pushcclosure(L_, lge_draw_rectangle, 1);
    lua_setfield(L_, -2, "draw_rectangle");

    // draw_triangle
    lua_pushlightuserdata(L_, self);
    lua_pushcclosure(L_, lge_draw_triangle, 1);
    lua_setfield(L_, -2, "draw_triangle");

    // draw_text
    lua_pushlightuserdata(L_, self);
    lua_pushcclosure(L_, lge_draw_text, 1);
    lua_setfield(L_, -2, "draw_text");

    // present
    lua_pushlightuserdata(L_, self);
    lua_pushcclosure(L_, lge_present, 1);
    lua_setfield(L_, -2, "present");

    // load_spritesheet
    lua_pushlightuserdata(L_, self);
    lua_pushcclosure(L_, lge_load_spritesheet, 1);
    lua_setfield(L_, -2, "load_spritesheet");

    // create_sprite
    lua_pushlightuserdata(L_, self);
    lua_pushcclosure(L_, lge_create_sprite, 1);
    lua_setfield(L_, -2, "create_sprite");

    lua_pushlightuserdata(L_, self);
    lua_pushcclosure(L_, lge_delay_ms, 1);
    lua_setfield(L_, -2, "delay");

    lua_pushlightuserdata(L_, self);
    lua_pushcclosure(L_, lge_fps, 1);
    lua_setfield(L_, -2, "fps");

    // touch management
    lua_pushlightuserdata(L_, self);
    lua_pushcclosure(L_, lge_get_mouse_click, 1);
    lua_setfield(L_, -2, "get_mouse_click");

    lua_pushlightuserdata(L_, self);
    lua_pushcclosure(L_, lge_get_mouse_position, 1);
    lua_setfield(L_, -2, "get_mouse_position");

    lua_pushlightuserdata(L_, self);
    lua_pushcclosure(L_, lge_is_key_down, 1);
    lua_setfield(L_, -2, "is_key_down");

    // --- 3D API ---

    // set_3d_camera(fov, cam_distance)
    lua_pushlightuserdata(L_, self);
    lua_pushcclosure(L_, lge_set_3d_camera, 1);
    lua_setfield(L_, -2, "set_3d_camera");

    // create_3d_model(vertices_flat, faces_flat)
    lua_pushlightuserdata(L_, self);
    lua_pushcclosure(L_, lge_create_3d_model, 1);
    lua_setfield(L_, -2, "create_3d_model");

    // create_3d_instance(model_id, tri_colors)
    lua_pushlightuserdata(L_, self);
    lua_pushcclosure(L_, lge_create_3d_instance, 1);
    lua_setfield(L_, -2, "create_3d_instance");

    // draw_3d_instance(instance_id, wx, wy, wz, radius, ax, ay, az)
    lua_pushlightuserdata(L_, self);
    lua_pushcclosure(L_, lge_draw_3d_instance, 1);
    lua_setfield(L_, -2, "draw_3d_instance");

    // set_3d_light(dx, dy, dz, ambient, diffuse)
    lua_pushlightuserdata(L_, self);
    lua_pushcclosure(L_, lge_set_3d_light, 1);
    lua_setfield(L_, -2, "set_3d_light");

    // --- WebSocket API ---

#if ENABLE_WIFI
    // ws_connect(url)
    lua_pushlightuserdata(L_, self);
    lua_pushcclosure(L_, lge_ws_connect, 1);
    lua_setfield(L_, -2, "ws_connect");

    // ws_send(message)
    lua_pushlightuserdata(L_, self);
    lua_pushcclosure(L_, lge_ws_send, 1);
    lua_setfield(L_, -2, "ws_send");

    // ws_on_message(callback)
    lua_pushlightuserdata(L_, self);
    lua_pushcclosure(L_, lge_ws_on_message, 1);
    lua_setfield(L_, -2, "ws_on_message");

    // ws_is_connected()
    lua_pushlightuserdata(L_, self);
    lua_pushcclosure(L_, lge_ws_is_connected, 1);
    lua_setfield(L_, -2, "ws_is_connected");

    // ws_disconnect()
    lua_pushlightuserdata(L_, self);
    lua_pushcclosure(L_, lge_ws_disconnect, 1);
    lua_setfield(L_, -2, "ws_disconnect");

    // ws_loop()
    lua_pushlightuserdata(L_, self);
    lua_pushcclosure(L_, lge_ws_loop, 1);
    lua_setfield(L_, -2, "ws_loop");
#endif

    // Set the table in the global namespace as `lge`
    lua_setglobal(L_, "lge");
}

// --- Lua C functions ---
static int luaLedControl(lua_State *L)
{
    const int pin = static_cast<int>(luaL_checkinteger(L, 1));
    const int state = static_cast<int>(luaL_checkinteger(L, 2));
    pinMode(pin, OUTPUT);
    digitalWrite(pin, state);
    Serial.printf("Lua: Set LED pin %d to %d\n", pin, state);
    return 0;
}

static int luaAnalogRead(lua_State *L)
{
    const int pin = static_cast<int>(luaL_checkinteger(L, 1));
    const int value = analogRead(pin);
    lua_pushinteger(L, value);
    Serial.printf("Lua: Read analog pin %d: %d\n", pin, value);
    return 1;
}

static int luaDelayMs(lua_State *L)
{
    const int ms = static_cast<int>(luaL_checkinteger(L, 1));
    delay(ms);
    return 0;
}

static int luaMillis(lua_State *L)
{
    lua_pushinteger(L, static_cast<lua_Integer>(millis()));
    return 1;
}

static int luaPrint(lua_State *L)
{
    const char *s = luaL_checkstring(L, 1);
    Serial.print("Lua print: ");
    Serial.println(s);
    return 0;
}

static int luaGetSystemInfo(lua_State *L)
{
    lua_createtable(L, 0, 4);
    lua_pushstring(L, "ESP32");
    lua_setfield(L, -2, "chip");
    lua_pushinteger(L, static_cast<lua_Integer>(ESP.getFreeHeap()));
    lua_setfield(L, -2, "free_heap");
    lua_pushinteger(L, static_cast<lua_Integer>(ESP.getCpuFreqMHz()));
    lua_setfield(L, -2, "cpu_freq");
    lua_pushinteger(L, static_cast<lua_Integer>(millis()));
    lua_setfield(L, -2, "uptime");
    return 1;
}

// --- LGE helper functions ---

void LuaDriver::addDirtyRect(int x, int y, int w, int h)
{
#if DIRTY_RECTS_OPTIMIZATION
    // 1. Basic validation and clipping
    if (w <= 0 || h <= 0 || !tft_)
        return;

    int x1_new = std::max(0, x);
    int y1_new = std::max(0, y);
    int x2_new = std::min(tft_->width() - 1, x + w - 1);
    int y2_new = std::min(tft_->height() - 1, y + h - 1);

    if (x1_new > x2_new || y1_new > y2_new)
        return;

    DirtyRect new_rect = {x1_new, y1_new, x2_new, y2_new};
    current_dirty_rects_.push_back(new_rect);
#endif
}

static inline int hexVal(char c)
{
    return (c <= '9')   ? c - '0'
           : (c <= 'F') ? c - 'A' + 10
           : (c <= 'f') ? c - 'a' + 10
                        : 0;
}

uint16_t LuaDriver::parseHexColor(const char *hex)
{
    if (!hex || hex[0] != '#' || strlen(hex) < 7)
        return TFT_WHITE;

    int r = hexVal(hex[1]) * 16 + hexVal(hex[2]);
    int g = hexVal(hex[3]) * 16 + hexVal(hex[4]);
    int b = hexVal(hex[5]) * 16 + hexVal(hex[6]);

    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

uint16_t LuaDriver::scaleColor565(uint16_t c, float factor)
{
    if (factor < 0.0f)
        factor = 0.0f;
    if (factor > 1.0f)
        factor = 1.0f;

    // Extract RGB 5/6/5
    uint8_t r5 = (c >> 11) & 0x1F;
    uint8_t g6 = (c >> 5) & 0x3F;
    uint8_t b5 = c & 0x1F;

    // Expand to 8-bit
    int r = (r5 * 255 + 15) / 31;
    int g = (g6 * 255 + 31) / 63;
    int b = (b5 * 255 + 15) / 31;

    // Scale
    r = (uint8_t)std::min(255.0f, r * factor);
    g = (uint8_t)std::min(255.0f, g * factor);
    b = (uint8_t)std::min(255.0f, b * factor);

    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

int LuaDriver::lge_clear_canvas(lua_State *L)
{
    LuaDriver *self = (LuaDriver *)lua_touserdata(L, lua_upvalueindex(1));
    static uint32_t lastColor = TFT_BLACK;
    if (self && self->spr_)
    {
        const char *hex = luaL_optstring(L, 1, "#000000");

        uint16_t color = self->parseHexColor(hex);
        self->spr_->fillScreen(color);

        // When clearing the whole screen, the whole screen is dirty!
        // This makes the next lge_present perform a full copy.
        if (color != lastColor)
        {
            Serial.printf("Clearing canvas with new color: %s (0x%04X)\n", hex, color);
            self->addDirtyRect(0, 0, self->spr_->width(), self->spr_->height());
            lastColor = color;
        }
    }
    return 0;
}

int LuaDriver::lge_delay_ms(lua_State *L)
{
    static int lastDelayTimestamp = 0;

    const int ms = static_cast<int>(luaL_checkinteger(L, 1));
    int currentMillis = millis();
    int timeSinceLastDelay = currentMillis - lastDelayTimestamp;
    int actualDelay = 0;
    if (timeSinceLastDelay < ms)
    {
        actualDelay = ms - timeSinceLastDelay;
    }
    delay(actualDelay);

#if DEBUG_DELAY_AVERAGE_PRINT
    static int totalDelay = 0;
    static int count = 0;
    totalDelay += actualDelay;
    if (++count >= 100)
    {
        Serial.printf("Average lge.delay() time over last %d calls: %.2f ms\n", count, (float)totalDelay / count);
        totalDelay = 0;
        count = 0;
    }
#endif

    lastDelayTimestamp = currentMillis;
    return 0;
}

int LuaDriver::lge_fps(lua_State *L)
{
    LuaDriver *self = (LuaDriver *)lua_touserdata(L, lua_upvalueindex(1));
    static unsigned long lastTime = 0;
    static int frameCount = 0;
    static float fps = 0;

    frameCount++;
    unsigned long currentTime = millis();
    unsigned long elapsed = currentTime - lastTime;

    if (elapsed >= 1000)
    {
        fps = (frameCount * 1000.0f) / elapsed;
        lastTime = currentTime;
        frameCount = 0;
    }

    lua_pushnumber(L, fps);
    return 1;
}

int LuaDriver::lge_get_canvas_size(lua_State *L)
{
    LuaDriver *self = (LuaDriver *)lua_touserdata(L, lua_upvalueindex(1));
    if (self && self->tft_)
    {
        lua_pushinteger(L, self->tft_->width());
        lua_pushinteger(L, self->tft_->height());
        return 2;
    }
    return 0;
}

int LuaDriver::lge_draw_circle(lua_State *L)
{
    LuaDriver *self = (LuaDriver *)lua_touserdata(L, lua_upvalueindex(1));
    if (self && self->spr_)
    {
        int x = (int)lua_tonumber(L, 1);
        int y = (int)lua_tonumber(L, 2);
        int r = (int)lua_tonumber(L, 3);
        const char *hex = luaL_optstring(L, 4, "#ffffff");
        uint16_t color = self->parseHexColor(hex);
        self->spr_->fillCircle(x, y, r, color);
        self->addDirtyRect(x - r, y - r, 2 * r + 1, 2 * r + 1);
    }

    return 0;
}

int LuaDriver::lge_draw_rectangle(lua_State *L)
{
    LuaDriver *self = (LuaDriver *)lua_touserdata(L, lua_upvalueindex(1));
    if (self && self->spr_)
    {
        int x = (int)lua_tonumber(L, 1);
        int y = (int)lua_tonumber(L, 2);
        int w = (int)lua_tonumber(L, 3);
        int h = (int)lua_tonumber(L, 4);
        const char *hex = luaL_optstring(L, 5, "#ffffff");
        uint16_t color = self->parseHexColor(hex);
        self->spr_->fillRect(x, y, w, h, color);
        self->addDirtyRect(x, y, w, h);
    }

    return 0;
}

int LuaDriver::lge_draw_triangle(lua_State *L)
{
    LuaDriver *self = (LuaDriver *)lua_touserdata(L, lua_upvalueindex(1));
    if (self && self->spr_)
    {
        int x0 = (int)lua_tonumber(L, 1);
        int y0 = (int)lua_tonumber(L, 2);
        int x1 = (int)lua_tonumber(L, 3);
        int y1 = (int)lua_tonumber(L, 4);
        int x2 = (int)lua_tonumber(L, 5);
        int y2 = (int)lua_tonumber(L, 6);
        const char *hex = luaL_optstring(L, 7, "#ffffff");
        uint16_t color = self->parseHexColor(hex);
        self->spr_->fillTriangle(x0, y0, x1, y1, x2, y2, color);

        // Compute bounding box for dirty rect
        int min_x = std::min({x0, x1, x2});
        int max_x = std::max({x0, x1, x2});
        int min_y = std::min({y0, y1, y2});
        int max_y = std::max({y0, y1, y2});
        self->addDirtyRect(min_x, min_y, max_x - min_x + 1, max_y - min_y + 1);
    }

    return 0;
}

int LuaDriver::lge_draw_text(lua_State *L)
{
    LuaDriver *self = (LuaDriver *)lua_touserdata(L, lua_upvalueindex(1));
    if (self && self->spr_)
    {
        int x = (int)luaL_checkinteger(L, 1);
        int y = (int)luaL_checkinteger(L, 2);
        const char *text = luaL_checkstring(L, 3);
        const char *hex = luaL_optstring(L, 4, "#ffffff");
        uint16_t color = self->parseHexColor(hex);

        self->spr_->setTextFont(2);
        self->spr_->setTextColor(color);
        self->spr_->setTextSize(1);

        // Get the bounding box BEFORE drawing
        int text_w = self->spr_->textWidth(text);
        int text_h = self->spr_->fontHeight();

        // 1. Draw to the internal sprite buffer
        self->spr_->drawString(text, x, y);

        // 2. MARK THE REGION AS DIRTY
        self->addDirtyRect(x, y, text_w, text_h);
    }
    return 0;
}

int LuaDriver::lge_present(lua_State *L)
{
#if DEBUG_PROFILING
    static int callCount = 0;
    static int totalTime = 0;
    static int dirtyRectTime = 0;
    int timer = millis();
#endif

    LuaDriver *self = (LuaDriver *)lua_touserdata(L, lua_upvalueindex(1));
    self->tft_->startWrite();
#if DIRTY_RECTS_OPTIMIZATION
    if (self && self->spr_)
    {
#if DEBUG_PROFILING
        int dirtyRectStart = millis();
#endif
        // 1. Combine ALL dirty rects (erase and draw) into one list
        std::vector<DirtyRect> combined_rects;
        combined_rects.reserve(self->current_dirty_rects_.size() + self->previous_dirty_rects_.size());
        combined_rects.insert(combined_rects.end(), self->current_dirty_rects_.begin(), self->current_dirty_rects_.end());
        combined_rects.insert(combined_rects.end(), self->previous_dirty_rects_.begin(), self->previous_dirty_rects_.end());

        // 2. Perform the merging optimization
        mergeDirtyRects(combined_rects);
#if DEBUG_PROFILING
        dirtyRectTime += millis() - dirtyRectStart;
#endif
        // 3. Push the final, minimal set of merged rectangles
        for (const auto &rect : combined_rects)
        {
            int w = rect.x2 - rect.x1 + 1;
            int h = rect.y2 - rect.y1 + 1;
            self->spr_->pushSprite(rect.x1, rect.y1, rect.x1, rect.y1, w, h);
        }

        self->previous_dirty_rects_.clear();
        self->previous_dirty_rects_.swap(self->current_dirty_rects_);
    }
#else
    if (self && self->spr_ && self->tft_)
    {
        // Full sprite to TFT copy
        self->spr_->pushSprite(0, 0);
    }
#endif
    self->tft_->endWrite();

#if DEBUG_PROFILING
    int time_taken = millis() - timer;
    totalTime += time_taken;
    if (++callCount >= 50)
    {
        Serial.printf("Average time per call lge_present in ms: %g. Dirty Rect Mgmt: %g\n", (totalTime) / float(callCount), (dirtyRectTime) / float(callCount));
        Serial.printf("Free/Total/MinFree (high watermark) Internal SRAM: %d/%d/%d bytes\n", ESP.getFreeHeap(), ESP.getHeapSize(), ESP.getMinFreeHeap());
        callCount = 0;
        totalTime = 0;
        dirtyRectTime = 0;
    }
#endif
    self->updateMouseClick();
    return 0;
}

int LuaDriver::lge_load_spritesheet(lua_State *L)
{
    Serial.println("lge.load_spritesheet: Not Implemented");
    lua_pushnil(L);
    return 1;
}

int LuaDriver::lge_create_sprite(lua_State *L)
{
    Serial.println("lge.create_sprite: Not Implemented");
    lua_pushnil(L);
    return 1;
}

// Update mouse click event from touch
void LuaDriver::updateMouseClick()
{
    if (ts_ && tft_ && ts_->tirqTouched() && ts_->touched())
    {
        TS_Point p = ts_->getPoint();
        int pixelX = map(p.x, TS_MIN_X_CONST, TS_MAX_X_CONST, 0, tft_->width());
        int pixelY = map(p.y, TS_MIN_Y_CONST, TS_MAX_Y_CONST, 0, tft_->height());
        mouse_click_.button = 0;
        mouse_click_.x = pixelX;
        mouse_click_.y = pixelY;
        mouse_click_.isTouchDown = true;
    }
    else
    {
        mouse_click_.isTouchDown = false;
        mouse_click_.isConsumed = false;
    }
}

// Lua binding: lge.get_mouse_click()
int LuaDriver::lge_get_mouse_click(lua_State *L)
{
    LuaDriver *self = (LuaDriver *)lua_touserdata(L, lua_upvalueindex(1));
    if (self && self->mouse_click_.isTouchDown && !self->mouse_click_.isConsumed)
    {
        lua_pushinteger(L, self->mouse_click_.button);
        lua_pushinteger(L, self->mouse_click_.x);
        lua_pushinteger(L, self->mouse_click_.y);
        self->mouse_click_.isConsumed = true;
        return 3;
    }
    else
    {
        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushnil(L);
        return 3;
    }
}

// Lua binding: lge.get_mouse_click()
int LuaDriver::lge_get_mouse_position(lua_State *L)
{
    LuaDriver *self = (LuaDriver *)lua_touserdata(L, lua_upvalueindex(1));
    if (self && self->mouse_click_.isTouchDown)
    {
        lua_pushinteger(L, self->mouse_click_.button);
        lua_pushinteger(L, self->mouse_click_.x);
        lua_pushinteger(L, self->mouse_click_.y);
        return 3;
    }
    else
    {
        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushnil(L);
        return 3;
    }
}

// Lua binding: lge.is_key_down()
int LuaDriver::lge_is_key_down(lua_State *L)
{
    LuaDriver *self = (LuaDriver *)lua_touserdata(L, lua_upvalueindex(1));
    if (self && self->isPressedCallback_)
    {
        const char *text = luaL_checkstring(L, 1);
        if (!text)
        {
            lua_pushboolean(L, false);
            return 1;
        }

        // Map text to ControllerButton enum
        KeyCode btn = KeyCode::KEY_NONE;
        if (strcasecmp(text, "up") == 0)
            btn = KeyCode::KEY_UP;
        else if (strcasecmp(text, "down") == 0)
            btn = KeyCode::KEY_DOWN;
        else if (strcasecmp(text, "left") == 0)
            btn = KeyCode::KEY_LEFT;
        else if (strcasecmp(text, "right") == 0)
            btn = KeyCode::KEY_RIGHT;

        bool isDown = self->isPressedCallback_(btn);
        lua_pushboolean(L, isDown);
        return 1;
    }
    else
    {
        lua_pushboolean(L, false);
        return 1;
    }
}

int LuaDriver::lge_set_3d_camera(lua_State *L)
{
    LuaDriver *self = (LuaDriver *)lua_touserdata(L, lua_upvalueindex(1));
    if (!self)
        return 0;

    self->fov3d_ = (float)luaL_checknumber(L, 1);
    self->camDist3d_ = (float)luaL_checknumber(L, 2);

    return 0;
}

int LuaDriver::lge_create_3d_model(lua_State *L)
{
    LuaDriver *self = (LuaDriver *)lua_touserdata(L, lua_upvalueindex(1));
    if (!self)
        return 0;

    luaL_checktype(L, 1, LUA_TTABLE); // vertices_flat
    luaL_checktype(L, 2, LUA_TTABLE); // faces_flat

    Model3D model;

    // --- Read vertices ---
    size_t vlen = lua_rawlen(L, 1);
    if (vlen % 3 != 0)
    {
        Serial.printf("lge.create_3d_model: vertex array length %u is not multiple of 3\n", (unsigned)vlen);
    }

    model.vertices.resize(vlen);
    for (size_t i = 0; i < vlen; ++i)
    {
        lua_rawgeti(L, 1, (int)(i + 1));
        model.vertices[i] = (float)luaL_checknumber(L, -1);
        lua_pop(L, 1);
    }

    // --- Read faces (indices) ---
    size_t flen = lua_rawlen(L, 2);
    if (flen % 3 != 0)
    {
        Serial.printf("lge.create_3d_model: face index array length %u is not multiple of 3\n", (unsigned)flen);
    }

    model.indices.resize(flen);
    for (size_t i = 0; i < flen; ++i)
    {
        lua_rawgeti(L, 2, (int)(i + 1));
        int idx1based = (int)luaL_checkinteger(L, -1);
        lua_pop(L, 1);

        if (idx1based <= 0)
        {
            idx1based = 1;
        }
        model.indices[i] = (uint16_t)(idx1based - 1); // convert to 0-based
    }

    self->models3d_.push_back(std::move(model));
    int modelId = (int)self->models3d_.size(); // 1-based handle for Lua

    lua_pushinteger(L, modelId);
    return 1;
}

int LuaDriver::lge_create_3d_instance(lua_State *L)
{
    LuaDriver *self = (LuaDriver *)lua_touserdata(L, lua_upvalueindex(1));
    if (!self)
        return 0;

    int modelId = (int)luaL_checkinteger(L, 1);
    luaL_checktype(L, 2, LUA_TTABLE); // tri_colors

    if (modelId <= 0 || modelId > (int)self->models3d_.size())
    {
        return luaL_error(L, "lge.create_3d_instance: invalid model id %d", modelId);
    }

    Instance3D instance;
    instance.modelIndex = modelId - 1;

    const Model3D &model = self->models3d_[instance.modelIndex];
    size_t faceCount = model.indices.size() / 3;

    size_t clen = lua_rawlen(L, 2);
    if (clen < faceCount)
    {
        Serial.printf("lge.create_3d_instance: got %u colors for %u faces, padding with white\n",
                      (unsigned)clen, (unsigned)faceCount);
    }

    instance.faceColors565.resize(faceCount);

    for (size_t i = 0; i < faceCount; ++i)
    {
        uint16_t color565 = TFT_WHITE;

        if (i < clen)
        {
            lua_rawgeti(L, 2, (int)(i + 1));
            const char *hex = luaL_checkstring(L, -1);
            lua_pop(L, 1);
            color565 = self->parseHexColor(hex);
        }

        instance.faceColors565[i] = color565;
    }

    self->instances3d_.push_back(std::move(instance));
    int instanceId = (int)self->instances3d_.size(); // 1-based for Lua

    lua_pushinteger(L, instanceId);
    return 1;
}

int LuaDriver::lge_draw_3d_instance(lua_State *L)
{
    LuaDriver *self = (LuaDriver *)lua_touserdata(L, lua_upvalueindex(1));
    if (!self || !self->spr_)
        return 0;

    int instanceId = (int)luaL_checkinteger(L, 1);
    float wx = (float)luaL_checknumber(L, 2);     // world x
    float wy = (float)luaL_checknumber(L, 3);     // world y
    float wz = (float)luaL_checknumber(L, 4);     // world z (distance from camera)
    float radius = (float)luaL_checknumber(L, 5); // desired approx 2D radius at depth wz
    float ax = (float)luaL_checknumber(L, 6);
    float ay = (float)luaL_checknumber(L, 7);
    float az = (float)luaL_checknumber(L, 8);

    if (instanceId <= 0 || instanceId > (int)self->instances3d_.size())
        return 0;

    Instance3D &inst = self->instances3d_[instanceId - 1];
    if (inst.modelIndex < 0 || inst.modelIndex >= (int)self->models3d_.size())
        return 0;

    Model3D &model = self->models3d_[inst.modelIndex];

    const auto &srcVerts = model.vertices;
    const auto &indices = model.indices;

    size_t vertCount = srcVerts.size() / 3;
    size_t faceCount = indices.size() / 3;

    if (vertCount == 0 || faceCount == 0)
        return 0;

    // Ensure scratch buffers are large enough
    // We now store 6 floats per vertex:
    // [0]=camX, [1]=camY, [2]=camZ, [3]=screenX, [4]=screenY, [5]=unused
    self->tempVertices3d_.resize(vertCount * 6);
    self->visibleZ_.resize(faceCount);
    self->visibleB1_.resize(faceCount);
    self->visibleB2_.resize(faceCount);
    self->visibleB3_.resize(faceCount);
    self->visibleColor_.resize(faceCount);

    const float fov = self->fov3d_;

    // Prevent division by zero / behind-camera placement
    if (wz < 1.0f)
        wz = 1.0f;

    // Approximate scale so that projected radius ~ 'radius' at depth ~ wz
    // visual_radius ≈ scale * (fov / wz)  =>  scale ≈ radius * (wz / fov)
    float baseScale = radius * (wz / fov);

    // Precompute trig
    float cos_x = std::cos(ax);
    float sin_x = std::sin(ax);
    float cos_y = std::cos(ay);
    float sin_y = std::sin(ay);
    float cos_z = std::cos(az);
    float sin_z = std::sin(az);

    // Projection center = center of sprite
    float centerX = self->spr_->width() * 0.5f;
    float centerY = self->spr_->height() * 0.5f;

    // 1) Transform all vertices: model -> scaled -> rotated -> world -> camera -> screen
    for (size_t i = 0; i < vertCount; ++i)
    {
        size_t srcBase = i * 3;
        size_t tmpBase = i * 6;

        // Model-space
        float vx = srcVerts[srcBase + 0] * baseScale;
        float vy = srcVerts[srcBase + 1] * baseScale;
        float vz = srcVerts[srcBase + 2] * baseScale;

        // Rotate: Rx, then Ry, then Rz (same order as your Lua pipeline)
        float y1 = vy * cos_x - vz * sin_x;
        float z1 = vy * sin_x + vz * cos_x;
        float x1 = vx;

        float x2 = x1 * cos_y + z1 * sin_y;
        float z2 = -x1 * sin_y + z1 * cos_y;
        float y2 = y1;

        float x3 = x2 * cos_z - y2 * sin_z;
        float y3 = x2 * sin_z + y2 * cos_z;
        float z3 = z2;

        // World space: translate by (wx, wy, wz)
        float camX = x3 + wx;
        float camY = y3 + wy;
        float camZ = z3 + wz; // camera at origin, looking along +Z

        if (camZ < 0.001f)
            camZ = 0.001f;

        float z_factor = fov / camZ;

        float sx = camX * z_factor + centerX;
        float sy = camY * z_factor + centerY;

        // Store camera-space coords
        self->tempVertices3d_[tmpBase + 0] = camX;
        self->tempVertices3d_[tmpBase + 1] = camY;
        self->tempVertices3d_[tmpBase + 2] = camZ;

        // Store screen-space coords
        self->tempVertices3d_[tmpBase + 3] = sx;
        self->tempVertices3d_[tmpBase + 4] = sy;
    }

    // 2) Back-face culling + visible face pool
    int visCount = 0;

    for (size_t f = 0; f < faceCount; ++f)
    {
        int i1 = indices[f * 3 + 0];
        int i2 = indices[f * 3 + 1];
        int i3 = indices[f * 3 + 2];

        int b1 = i1 * 6;
        int b2 = i2 * 6;
        int b3 = i3 * 6;

        float sx1 = self->tempVertices3d_[b1 + 3];
        float sy1 = self->tempVertices3d_[b1 + 4];
        float sz1 = self->tempVertices3d_[b1 + 2];

        float sx2 = self->tempVertices3d_[b2 + 3];
        float sy2 = self->tempVertices3d_[b2 + 4];
        float sz2 = self->tempVertices3d_[b2 + 2];

        float sx3 = self->tempVertices3d_[b3 + 3];
        float sy3 = self->tempVertices3d_[b3 + 4];
        float sz3 = self->tempVertices3d_[b3 + 2];

        // screen-space 2D cross product for back-face culling
        float normal_z = (sx2 - sx1) * (sy3 - sy1) - (sy2 - sy1) * (sx3 - sx1);
        if (normal_z >= 0.0f)
        {
            continue; // back face
        }

        float avgZ = (sz1 + sz2 + sz3) * (1.0f / 3.0f);

        uint16_t col = TFT_WHITE;
        if (f < inst.faceColors565.size())
            col = inst.faceColors565[f];

        uint16_t finalCol = col;

        if (self->lightEnabled_)
        {
            // Use camera-space positions directly for face normal
            float cx1 = self->tempVertices3d_[b1 + 0];
            float cy1 = self->tempVertices3d_[b1 + 1];
            float cz1 = self->tempVertices3d_[b1 + 2];

            float cx2 = self->tempVertices3d_[b2 + 0];
            float cy2 = self->tempVertices3d_[b2 + 1];
            float cz2 = self->tempVertices3d_[b2 + 2];

            float cx3 = self->tempVertices3d_[b3 + 0];
            float cy3 = self->tempVertices3d_[b3 + 1];
            float cz3 = self->tempVertices3d_[b3 + 2];

            // e1 = p2 - p1, e2 = p3 - p1
            float e1x = cx2 - cx1;
            float e1y = cy2 - cy1;
            float e1z = cz2 - cz1;

            float e2x = cx3 - cx1;
            float e2y = cy3 - cy1;
            float e2z = cz3 - cz1;

            // normal = e1 x e2
            float nx = e1y * e2z - e1z * e2y;
            float ny = e1z * e2x - e1x * e2z;
            float nz = e1x * e2y - e1y * e2x;

            float nlen = std::sqrt(nx * nx + ny * ny + nz * nz);
            float ndotl = 0.0f;
            if (nlen > 1e-4f)
            {
                nx /= nlen;
                ny /= nlen;
                nz /= nlen;

                ndotl = nx * self->lightDirX_ +
                        ny * self->lightDirY_ +
                        nz * self->lightDirZ_;
            }

            if (ndotl < 0.0f)
                ndotl = 0.0f; // Lambert – no negative light

            float brightness = self->lightAmbient_ + self->lightDiffuse_ * ndotl;
            if (brightness > 1.0f)
                brightness = 1.0f;
            if (brightness < 0.0f)
                brightness = 0.0f;

            finalCol = scaleColor565(col, brightness);
        }

        self->visibleZ_[visCount] = avgZ;
        self->visibleB1_[visCount] = b1;
        self->visibleB2_[visCount] = b2;
        self->visibleB3_[visCount] = b3;
        self->visibleColor_[visCount] = finalCol;
        ++visCount;
    }

    // 3) Sort visible faces by Z (descending) – insertion sort (few faces, cheap)
    for (int i = 1; i < visCount; ++i)
    {
        float zkey = self->visibleZ_[i];
        int k_b1 = self->visibleB1_[i];
        int k_b2 = self->visibleB2_[i];
        int k_b3 = self->visibleB3_[i];
        uint16_t k_color = self->visibleColor_[i];

        int j = i - 1;
        while (j >= 0 && self->visibleZ_[j] < zkey)
        {
            self->visibleZ_[j + 1] = self->visibleZ_[j];
            self->visibleB1_[j + 1] = self->visibleB1_[j];
            self->visibleB2_[j + 1] = self->visibleB2_[j];
            self->visibleB3_[j + 1] = self->visibleB3_[j];
            self->visibleColor_[j + 1] = self->visibleColor_[j];
            --j;
        }
        self->visibleZ_[j + 1] = zkey;
        self->visibleB1_[j + 1] = k_b1;
        self->visibleB2_[j + 1] = k_b2;
        self->visibleB3_[j + 1] = k_b3;
        self->visibleColor_[j + 1] = k_color;
    }

    // 4) Draw visible faces
    int dirtyRectMinX = self->spr_->width();
    int dirtyRectMinY = self->spr_->height();
    int dirtyRectMaxX = 0;
    int dirtyRectMaxY = 0;
    for (int i = 0; i < visCount; ++i)
    {
        int b1 = self->visibleB1_[i];
        int b2 = self->visibleB2_[i];
        int b3 = self->visibleB3_[i];

        int x0 = (int)self->tempVertices3d_[b1 + 3];
        int y0 = (int)self->tempVertices3d_[b1 + 4];
        int x1 = (int)self->tempVertices3d_[b2 + 3];
        int y1 = (int)self->tempVertices3d_[b2 + 4];
        int x2 = (int)self->tempVertices3d_[b3 + 3];
        int y2 = (int)self->tempVertices3d_[b3 + 4];

        uint16_t color = self->visibleColor_[i];

        self->spr_->fillTriangle(x0, y0, x1, y1, x2, y2, color);

        // Mark dirty region for partial update
        int min_x = std::min({x0, x1, x2});
        int max_x = std::max({x0, x1, x2});
        int min_y = std::min({y0, y1, y2});
        int max_y = std::max({y0, y1, y2});
        if (min_x < dirtyRectMinX)
            dirtyRectMinX = min_x;
        if (min_y < dirtyRectMinY)
            dirtyRectMinY = min_y;
        if (max_x > dirtyRectMaxX)
            dirtyRectMaxX = max_x;
        if (max_y > dirtyRectMaxY)
            dirtyRectMaxY = max_y;
    }

    self->addDirtyRect(dirtyRectMinX, dirtyRectMinY, dirtyRectMaxX - dirtyRectMinX + 1, dirtyRectMaxY - dirtyRectMinY + 1);

    return 0;
}

int LuaDriver::lge_set_3d_light(lua_State *L)
{
    LuaDriver *self = (LuaDriver *)lua_touserdata(L, lua_upvalueindex(1));
    if (!self)
        return 0;

    float dx = (float)luaL_checknumber(L, 1);          // Direction X - Higher is right
    float dy = (float)luaL_checknumber(L, 2);          // Direction Y - Higher is up
    float dz = (float)luaL_checknumber(L, 3);          // Direction Z - Higher is deeper into screen
    float ambient = (float)luaL_optnumber(L, 4, 0.2f); // Ambient [0..1], the higher, the brighter
    float diffuse = (float)luaL_optnumber(L, 5, 0.8f); // Diffuse [0..1], the higher, the stronger the light - Should sum to 1.0 with ambient

    float len = std::sqrt(dx * dx + dy * dy + dz * dz);
    if (len < 1e-4f)
    {
        // disable lighting if direction is zero
        self->lightEnabled_ = false;
        return 0;
    }

    dy *= -1.0f; // Invert Y to match screen coords

    self->lightDirX_ = dx / len;
    self->lightDirY_ = dy / len;
    self->lightDirZ_ = dz / len;

    // clamp to [0,1]
    if (ambient < 0.0f)
        ambient = 0.0f;
    if (ambient > 1.0f)
        ambient = 1.0f;
    if (diffuse < 0.0f)
        diffuse = 0.0f;
    if (diffuse > 1.0f)
        diffuse = 1.0f;

    self->lightAmbient_ = ambient;
    self->lightDiffuse_ = diffuse;
    self->lightEnabled_ = true;

    return 0;
}

// --- WebSocket Implementation ---
#if ENABLE_WIFI
void LuaDriver::webSocketEvent(LuaDriver *self, WStype_t type, uint8_t *payload, size_t length)
{
    if (self)
    {
        self->handleWebSocketEvent(type, payload, length);
    }
}

void LuaDriver::handleWebSocketEvent(WStype_t type, uint8_t *payload, size_t length)
{
    switch (type)
    {
    case WStype_DISCONNECTED:
        Serial.println("[WS] Disconnected");
        wsConnected_ = false;
        break;

    case WStype_CONNECTED:
        Serial.printf("[WS] Connected to url: %s\n", payload);
        wsConnected_ = true;
        break;

    case WStype_TEXT:
    {
        Serial.printf("[WS] Received: %s\n", payload);

        // Call Lua callback if registered
        if (wsCallbackRef_ != LUA_NOREF && L_)
        {
            // Get the callback function from registry
            lua_rawgeti(L_, LUA_REGISTRYINDEX, wsCallbackRef_);

            if (lua_isfunction(L_, -1))
            {
                // Parse JSON message
                JsonDocument doc;
                DeserializationError error = deserializeJson(doc, payload, length);

                if (!error)
                {
                    // Create Lua table for the message
                    lua_newtable(L_);

                    // Add message type if present
                    if (doc["type"].is<const char *>())
                    {
                        const char *typeStr = doc["type"];
                        lua_pushstring(L_, typeStr);
                        lua_setfield(L_, -2, "type");
                    }

                    // Add all other fields from JSON
                    for (JsonPair kv : doc.as<JsonObject>())
                    {
                        const char *key = kv.key().c_str();
                        JsonVariant value = kv.value();

                        lua_pushstring(L_, key);

                        if (value.is<const char *>())
                        {
                            lua_pushstring(L_, value.as<const char *>());
                        }
                        else if (value.is<int>())
                        {
                            lua_pushinteger(L_, value.as<int>());
                        }
                        else if (value.is<double>())
                        {
                            lua_pushnumber(L_, value.as<double>());
                        }
                        else if (value.is<bool>())
                        {
                            lua_pushboolean(L_, value.as<bool>());
                        }
                        else if (value.isNull())
                        {
                            lua_pushnil(L_);
                        }
                        else
                        {
                            // For complex types, push as string representation
                            String str;
                            serializeJson(value, str);
                            lua_pushstring(L_, str.c_str());
                        }

                        lua_settable(L_, -3);
                    }

                    // Call the Lua callback with the message table
                    if (lua_pcall(L_, 1, 0, 0) != LUA_OK)
                    {
                        Serial.print("[WS] Lua callback error: ");
                        Serial.println(lua_tostring(L_, -1));
                        lua_pop(L_, 1);
                    }
                }
                else
                {
                    Serial.printf("[WS] JSON parse error: %s\n", error.c_str());

                    // Call callback with raw string if JSON parsing fails
                    lua_pushstring(L_, (const char *)payload);

                    if (lua_pcall(L_, 1, 0, 0) != LUA_OK)
                    {
                        Serial.print("[WS] Lua callback error: ");
                        Serial.println(lua_tostring(L_, -1));
                        lua_pop(L_, 1);
                    }
                }
            }
            else
            {
                lua_pop(L_, 1); // Pop non-function value
            }
        }
    }
    break;

    case WStype_ERROR:
        Serial.printf("[WS] Error: %s\n", payload);
        break;

    case WStype_BIN:
        Serial.println("[WS] Binary data received (not supported)");
        break;

    default:
        break;
    }
}

// lge.ws_connect(url) - Connect to WebSocket server
int LuaDriver::lge_ws_connect(lua_State *L)
{
    LuaDriver *self = (LuaDriver *)lua_touserdata(L, lua_upvalueindex(1));
    if (!self)
        return 0;

    const char *url = luaL_checkstring(L, 1);
    if (!url)
    {
        lua_pushboolean(L, false);
        return 1;
    }

    // Initialize WiFi once if not already done
    if (!self->wifiInitialized_ && self->wifiInitCallback_)
    {
        Serial.println("[WS] Initializing WiFi...");
        self->wifiInitCallback_();
        self->wifiInitialized_ = true;
    }

    Serial.printf("[WS] Connecting to: %s\n", url);

    // Parse URL: ws://host:port/path or wss://host:port/path
    String urlStr(url);
    bool isSecure = urlStr.startsWith("wss://");
    bool isInsecure = urlStr.startsWith("ws://");

    if (!isSecure && !isInsecure)
    {
        Serial.println("[WS] Invalid URL scheme. Use ws:// or wss://");
        lua_pushboolean(L, false);
        return 1;
    }

    // Remove protocol
    String remaining = urlStr.substring(isSecure ? 6 : 5);

    // Find host, port, and path
    int slashPos = remaining.indexOf('/');
    String hostPort = slashPos >= 0 ? remaining.substring(0, slashPos) : remaining;
    String path = slashPos >= 0 ? remaining.substring(slashPos) : "/";

    int colonPos = hostPort.indexOf(':');
    String host = colonPos >= 0 ? hostPort.substring(0, colonPos) : hostPort;
    uint16_t port = colonPos >= 0 ? hostPort.substring(colonPos + 1).toInt() : (isSecure ? 443 : 80);

    Serial.printf("[WS] Parsed - Host: %s, Port: %d, Path: %s, Secure: %d\n",
                  host.c_str(), port, path.c_str(), isSecure);

    // Set up event handler with lambda that captures 'self'
    self->wsClient_.onEvent([self](WStype_t type, uint8_t *payload, size_t length)
                            { LuaDriver::webSocketEvent(self, type, payload, length); });

    // Connect
    if (isSecure)
    {
        self->wsClient_.beginSSL(host, port, path);
    }
    else
    {
        self->wsClient_.begin(host, port, path);
    }

    self->wsClient_.setReconnectInterval(5000);

    lua_pushboolean(L, true);
    return 1;
}

// lge.ws_send(message) - Send message to WebSocket server
int LuaDriver::lge_ws_send(lua_State *L)
{
    LuaDriver *self = (LuaDriver *)lua_touserdata(L, lua_upvalueindex(1));
    if (!self || !self->wsConnected_)
    {
        lua_pushboolean(L, false);
        return 1;
    }

    const char *message = luaL_checkstring(L, 1);
    if (!message)
    {
        lua_pushboolean(L, false);
        return 1;
    }

    Serial.printf("[WS] Sending: %s\n", message);
    bool success = self->wsClient_.sendTXT(message);

    lua_pushboolean(L, success);
    return 1;
}

// lge.ws_on_message(callback) - Register Lua callback for WebSocket messages
int LuaDriver::lge_ws_on_message(lua_State *L)
{
    LuaDriver *self = (LuaDriver *)lua_touserdata(L, lua_upvalueindex(1));
    if (!self)
        return 0;

    // Check if argument is a function
    luaL_checktype(L, 1, LUA_TFUNCTION);

    // Unref previous callback if exists
    if (self->wsCallbackRef_ != LUA_NOREF)
    {
        luaL_unref(L, LUA_REGISTRYINDEX, self->wsCallbackRef_);
    }

    // Store new callback in registry
    lua_pushvalue(L, 1); // Push the function to top of stack
    self->wsCallbackRef_ = luaL_ref(L, LUA_REGISTRYINDEX);

    Serial.println("[WS] Message callback registered");
    return 0;
}

// lge.ws_is_connected() - Check if WebSocket is connected
int LuaDriver::lge_ws_is_connected(lua_State *L)
{
    LuaDriver *self = (LuaDriver *)lua_touserdata(L, lua_upvalueindex(1));
    if (!self)
    {
        lua_pushboolean(L, false);
        return 1;
    }

    lua_pushboolean(L, self->wsConnected_);
    return 1;
}

// lge.ws_disconnect() - Disconnect from WebSocket server
int LuaDriver::lge_ws_disconnect(lua_State *L)
{
    LuaDriver *self = (LuaDriver *)lua_touserdata(L, lua_upvalueindex(1));
    if (!self)
        return 0;

    Serial.println("[WS] Disconnecting...");
    self->wsClient_.disconnect();
    self->wsConnected_ = false;

    return 0;
}

// lge.ws_loop() - Process WebSocket events (must be called regularly)
int LuaDriver::lge_ws_loop(lua_State *L)
{
    LuaDriver *self = (LuaDriver *)lua_touserdata(L, lua_upvalueindex(1));
    if (!self)
        return 0;

    self->wsClient_.loop();
    return 0;
}
#endif
