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

// C-style Lua hooks (need C linkage compatible signatures)
static int luaLedControl(lua_State *L);
static int luaAnalogRead(lua_State *L);
static int luaDelayMs(lua_State *L);
static int luaMillis(lua_State *L);
static int luaPrint(lua_State *L);
static int luaGetSystemInfo(lua_State *L);

// lge helper and bindings
// static int lge_clear_canvas(lua_State *L);
// static int lge_get_canvas_size(lua_State *L);
// static int lge_draw_circle(lua_State *L);
// static int lge_draw_text(lua_State *L);
// static int lge_present(lua_State *L);
// static int lge_load_spritesheet(lua_State *L);
// static int lge_create_sprite(lua_State *L);

// Implement LuaDriver methods

LuaDriver::LuaDriver(TFT_eSPI *tft, XPT2046_Touchscreen *ts)
    : L_(nullptr), led_status_(0), tft_(tft), ts_(ts)
{
}

LuaDriver::~LuaDriver()
{
    if (L_)
        lua_close(L_);
}

void LuaDriver::begin()
{
    Serial.println("ESP32 Lua Interpreter Starting...");
    L_ = luaL_newstate();
    if (!L_)
    {
        Serial.println("Failed to create Lua state");
        return;
    }
    luaL_openlibs(L_);
    registerFunctions();
}

void LuaDriver::loop()
{
    snprintf(lua_code_, sizeof(lua_code_), R"(
        if not ball then
            ball = { x = 60, y = 60, dx = 3, dy = 2, r = 20 }
        end
        local w, h = lge.get_canvas_size()

        while true do
            lge.clear_canvas()

            ball.x = ball.x + ball.dx
            ball.y = ball.y + ball.dy

            if (ball.x - ball.r < 0 and ball.dx < 0) or (ball.x + ball.r > w and ball.dx > 0) then
                ball.dx = -ball.dx
            end
            if (ball.y - ball.r < 0 and ball.dy < 0) or (ball.y + ball.r > h and ball.dy > 0) then
                ball.dy = -ball.dy
            end

            lge.draw_circle(ball.x, ball.y, ball.r, "#00aaff")
            lge.draw_text(50, 50, "Bouncing Ball", "#ffffff")
            
            lge.present()
            lge.delay(40)
        end
    )");

    const int result = luaL_dostring(L_, lua_code_);
    if (result != LUA_OK)
    {
        Serial.print("Error in periodic script: ");
        Serial.println(lua_tostring(L_, -1));
        lua_pop(L_, 1);
    }

    delay(1000);
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

    // Also allow lge.delay as an alias for delay_ms
    lua_pushcfunction(L_, luaDelayMs);
    lua_setfield(L_, -2, "delay");

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
    if (self && self->tft_)
        self->tft_->fillScreen(TFT_BLACK);
    return 0;
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
    if (self && self->tft_)
    {
        int x = (int)luaL_checkinteger(L, 1);
        int y = (int)luaL_checkinteger(L, 2);
        int r = (int)luaL_checkinteger(L, 3);
        const char *hex = luaL_optstring(L, 4, "#ffffff");
        uint16_t color = parseHexColor(hex);
        self->tft_->fillCircle(x, y, r, color);
    }
    return 0;
}

int LuaDriver::lge_draw_text(lua_State *L)
{
    LuaDriver *self = (LuaDriver *)lua_touserdata(L, lua_upvalueindex(1));
    if (self && self->tft_)
    {
        int x = (int)luaL_checkinteger(L, 1);
        int y = (int)luaL_checkinteger(L, 2);
        const char *text = luaL_checkstring(L, 3);
        const char *hex = luaL_optstring(L, 4, "#ffffff");
        uint16_t color = parseHexColor(hex);
        self->tft_->setTextFont(2);
        self->tft_->setTextColor(color);
        self->tft_->drawString(text, x, y);
    }
    return 0;
}

int LuaDriver::lge_present(lua_State *L)
{
    (void)L;
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
