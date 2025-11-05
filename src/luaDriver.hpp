#pragma once
#ifndef LUA_DRIVER_HPP
#define LUA_DRIVER_HPP

// Forward-declare Lua state type to avoid including Lua headers in this header.
struct lua_State;

#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>

class LuaDriver
{
public:
    // Lifecycle
    LuaDriver(TFT_eSPI *tft, XPT2046_Touchscreen *ts);
    ~LuaDriver();

    // Initialize Lua runtime and run startup scripts
    void begin();

    // Called periodically from Arduino loop
    void loop();

    // Access underlying lua_State* if needed
    lua_State *state();

    // Example: call a Lua callback from C++
    void callLuaFunctionFromCpp();

private:
    // Internal implementation details
    lua_State *L_;
    int led_status_;
    char lua_code_[2048];

    TFT_eSPI *tft_;
    XPT2046_Touchscreen *ts_;

    void registerFunctions();
    void registerLgeModule();
    static int lge_clear_canvas(lua_State *L);
    static int lge_get_canvas_size(lua_State *L);
    static int lge_draw_circle(lua_State *L);
    static int lge_draw_text(lua_State *L);
    static int lge_present(lua_State *L);
    static int lge_load_spritesheet(lua_State *L);
    static int lge_create_sprite(lua_State *L);
    static uint16_t parseHexColor(const char *hex);
};

#endif // LUA_DRIVER_HPP