#pragma once
#ifndef LUA_DRIVER_HPP
#define LUA_DRIVER_HPP

// Forward-declare Lua state type to avoid including Lua headers in this header.
struct lua_State;

#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include <vector>
#include "dirtyRects.hpp"

class LuaDriver
{
public:
    LuaDriver(TFT_eSPI *tft, XPT2046_Touchscreen *ts);
    ~LuaDriver();

    void begin();
    void loop();
    lua_State *state();
    void callLuaFunctionFromCpp();

private:
    lua_State *L_;
    int led_status_;
    char lua_code_[2048];
    TFT_eSPI *tft_;
    XPT2046_Touchscreen *ts_;
    TFT_eSprite *spr_;

    void registerFunctions();
    void registerLgeModule();

    static int lge_clear_canvas(lua_State *L);
    static int lge_get_canvas_size(lua_State *L);
    static int lge_draw_circle(lua_State *L);
    static int lge_draw_text(lua_State *L);
    static int lge_present(lua_State *L);
    static int lge_load_spritesheet(lua_State *L);
    static int lge_create_sprite(lua_State *L);
    static int lge_delay_ms(lua_State *L);
    static int lge_fps(lua_State *L);
    static uint16_t parseHexColor(const char *hex);

    std::vector<DirtyRect> current_dirty_rects_;
    std::vector<DirtyRect> previous_dirty_rects_;
    void addDirtyRect(int x, int y, int w, int h);
};

#endif // LUA_DRIVER_HPP