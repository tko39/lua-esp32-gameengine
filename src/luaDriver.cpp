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

#include "luaDriver.hpp"
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include <cstdio>
#include <algorithm>

#include "luaScript.h"
#include <memory>
#include <SPIFFS.h>
#include "flags.h"

// C-style Lua hooks (need C linkage compatible signatures)
static int luaLedControl(lua_State *L);
static int luaAnalogRead(lua_State *L);
static int luaDelayMs(lua_State *L);
static int luaMillis(lua_State *L);
static int luaPrint(lua_State *L);
static int luaGetSystemInfo(lua_State *L);

// Implement LuaDriver methods
LuaDriver::LuaDriver(TFT_eSPI *tft, XPT2046_Touchscreen *ts)
    : L_(nullptr), led_status_(0), tft_(tft), ts_(ts), spr_(nullptr)
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

    spr_->fillScreen(TFT_BLACK);
    spr_->setTextColor(TFT_WHITE, TFT_BLACK);
    spr_->setTextSize(2);

    spr_->drawString("Select a Lua script to run:\n\n", 10, 10);
    for (int i = 0; i < numScripts; i++)
    {
        spr_->drawString(String(i + 1) + ": Script " + String(i + 1) + "\n", 10, 30 + i * 20);
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

// NEW: Implementation of the Dirty Rect Adder
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

uint16_t LuaDriver::parseHexColor(const char *hex)
{
    if (!hex)
        return TFT_WHITE;
    int rr = 255, gg = 255, bb = 255;
    if (hex[0] == '#')
    {
        unsigned int vr = 255, vg = 255, vb = 255;
        if (sscanf(hex + 1, "%02x%02x%02x", &vr, &vg, &vb) == 3)
        {
            rr = (int)vr;
            gg = (int)vg;
            bb = (int)vb;
        }
    }

    return ((rr & 0xF8) << 8) | ((gg & 0xFC) << 3) | (bb >> 3);
}

int LuaDriver::lge_clear_canvas(lua_State *L)
{
    LuaDriver *self = (LuaDriver *)lua_touserdata(L, lua_upvalueindex(1));
    if (self && self->spr_)
    {
        self->spr_->fillScreen(TFT_BLACK);

        // When clearing the whole screen, the whole screen is dirty!
        // This makes the next lge_present perform a full copy.
        // self->addDirtyRect(0, 0, self->spr_->width(), self->spr_->height());
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
        uint16_t color = parseHexColor(hex);

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
    int timer = millis();
#endif

    LuaDriver *self = (LuaDriver *)lua_touserdata(L, lua_upvalueindex(1));
    self->tft_->startWrite();
#if DIRTY_RECTS_OPTIMIZATION
    if (self && self->spr_)
    {
        // 1. Combine ALL dirty rects (erase and draw) into one list
        std::vector<DirtyRect> combined_rects;
        combined_rects.reserve(self->current_dirty_rects_.size() + self->previous_dirty_rects_.size());
        combined_rects.insert(combined_rects.end(), self->current_dirty_rects_.begin(), self->current_dirty_rects_.end());
        combined_rects.insert(combined_rects.end(), self->previous_dirty_rects_.begin(), self->previous_dirty_rects_.end());

        // 2. Perform the merging optimization
        mergeDirtyRects(combined_rects);

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
    if (++callCount % 100 == 0)
    {
        // Report average time for the optimized (partial) copy
        Serial.printf("Average time per call lge_present in ms: %g\n", (totalTime) / float(callCount));
        Serial.printf("Free/Total/MinFree (high watermark) Internal SRAM: %d/%d/%d bytes\n", ESP.getFreeHeap(), ESP.getHeapSize(), ESP.getMinFreeHeap());
        callCount = 0;
        totalTime = 0;
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
