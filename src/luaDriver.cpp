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

// C-style Lua hooks (need C linkage compatible signatures)
static int luaLedControl(lua_State *L);
static int luaAnalogRead(lua_State *L);
static int luaDelayMs(lua_State *L);
static int luaMillis(lua_State *L);
static int luaPrint(lua_State *L);
static int luaGetSystemInfo(lua_State *L);

// Implement LuaDriver methods
LuaDriver::LuaDriver() : L_(nullptr), led_status_(0)
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

    const char *lua_script = R"(
        print("Lua script started!")
        local led_pin = 4
        for i = 1, 3 do
          print("Blink " .. i .. " at time: " .. millis())
          led(led_pin, 1)
          delay_ms(250)
          led(led_pin, 0)
          delay_ms(250)
        end
        local analog_value = analog_read(34)
        print("Analog pin 34 value: " .. analog_value)
        print("Script completed!")

        local info = get_system_info()
        print("Chip: " .. info.chip)
        print("Free Heap: " .. info.free_heap .. " bytes")
        print("CPU Freq: " .. info.cpu_freq .. " MHz")
        print("Uptime: " .. info.uptime .. " ms")
    )";

    const int result = luaL_dostring(L_, lua_script);
    if (result != LUA_OK)
    {
        Serial.print("Lua Error: ");
        Serial.println(lua_tostring(L_, -1));
        lua_pop(L_, 1);
    }
}

void LuaDriver::loop()
{
    snprintf(lua_code_, sizeof(lua_code_), "led(16, %d)", led_status_);
    led_status_ = 1 - led_status_;

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
    lua_register(L_, "led", luaLedControl);
    lua_register(L_, "analog_read", luaAnalogRead);
    lua_register(L_, "delay_ms", luaDelayMs);
    lua_register(L_, "millis", luaMillis);
    lua_register(L_, "print", luaPrint);
    lua_register(L_, "get_system_info", luaGetSystemInfo);
}

// Global instance used by C wrappers
static LuaDriver luaDriver;

// C wrappers to preserve previous API
void setup_lua()
{
    luaDriver.begin();
}

void loop_lua()
{
    luaDriver.loop();
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
