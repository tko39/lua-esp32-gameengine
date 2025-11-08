#pragma once
#ifndef LUA_DRIVER_HPP
#define LUA_DRIVER_HPP

// Forward-declare Lua state type to avoid including Lua headers in this header.
struct lua_State;

#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include <vector>
#include "dirtyRects.hpp"
#include "flags.h"

// Mouse click event state
struct MouseClick
{
    int button; // Always 1 for touch
    int x;
    int y;
    bool isTouchDown;
    bool isConsumed;
};

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
    MouseClick mouse_click_ = {0, 0, 0, false, false};

    char *cached_lua_fs_buf = nullptr;
    size_t cached_lua_fs_sz = 0;

    void registerFunctions();
    void registerLgeModule();
    int runLuaFromLittleFS();

    static int lge_clear_canvas(lua_State *L);
    static int lge_get_canvas_size(lua_State *L);
    static int lge_draw_circle(lua_State *L);
    static int lge_draw_text(lua_State *L);
    static int lge_present(lua_State *L);
    static int lge_load_spritesheet(lua_State *L);
    static int lge_create_sprite(lua_State *L);
    static int lge_delay_ms(lua_State *L);
    static int lge_fps(lua_State *L);
    static int lge_get_mouse_click(lua_State *L);
    static int lge_get_mouse_position(lua_State *L);
    static uint16_t parseHexColor(const char *hex);

    std::vector<DirtyRect> current_dirty_rects_;
    std::vector<DirtyRect> previous_dirty_rects_;
    void addDirtyRect(int x, int y, int w, int h);

    void updateMouseClick();

    static constexpr int TS_MIN_X_CONST = 240;
    static constexpr int TS_MAX_X_CONST = 3780;
    static constexpr int TS_MIN_Y_CONST = 220;
    static constexpr int TS_MAX_Y_CONST = 3800;
    static constexpr const char *RUNTIME_LUA_FILE = "/runtime.luac";
};

#endif // LUA_DRIVER_HPP