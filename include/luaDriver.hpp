#pragma once
#ifndef LUA_DRIVER_HPP
#define LUA_DRIVER_HPP

// Forward-declare Lua state type to avoid including Lua headers in this header.
struct lua_State;

#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include <vector>
#include "flags.h"
#if DIRTY_RECTS_OPTIMIZATION
#include "dirtyRects.hpp"
#elif DIRTY_TILE_OPTIMIZATION
#include "dirtyTiles.hpp"
#endif
#include "controller.hpp"
#if ENABLE_WIFI
#include <WebSocketsClient.h>
typedef void (*WiFiInitCallback)();
#endif

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
    void loadLuaFromFS();
    void loop();
    lua_State *state();
    void callLuaFunctionFromCpp();
    void setIsKeyDownCallback(KeyPressedCallback callback);
#if ENABLE_WIFI
    void setWiFiInitCallback(WiFiInitCallback callback);
#endif

private:
    lua_State *L_;
    int led_status_;
    char lua_code_[2048];
    TFT_eSPI *tft_;
    XPT2046_Touchscreen *ts_;
    TFT_eSprite *spr_;
    MouseClick mouse_click_ = {0, 0, 0, false, false};
    KeyPressedCallback isPressedCallback_ = nullptr;

    char *cached_lua_fs_buf = nullptr;
    size_t cached_lua_fs_sz = 0;

    void registerFunctions();
    void registerLgeModule();
    int runLuaFromFS();
    int scriptSelectionMenu();

    static int lge_clear_canvas(lua_State *L);
    static int lge_get_canvas_size(lua_State *L);
    static int lge_draw_circle(lua_State *L);
    static int lge_draw_rectangle(lua_State *L);
    static int lge_draw_triangle(lua_State *L);
    static int lge_draw_text(lua_State *L);
    static int lge_present(lua_State *L);
    static int lge_load_spritesheet(lua_State *L);
    static int lge_create_sprite(lua_State *L);
    static int lge_delay_ms(lua_State *L);
    static int lge_fps(lua_State *L);
    static int lge_get_mouse_click(lua_State *L);
    static int lge_get_mouse_position(lua_State *L);
    static int lge_is_key_down(lua_State *L);

// WebSocket functions
#if ENABLE_WIFI
    static int lge_ws_connect(lua_State *L);
    static int lge_ws_send(lua_State *L);
    static int lge_ws_on_message(lua_State *L);
    static int lge_ws_is_connected(lua_State *L);
    static int lge_ws_disconnect(lua_State *L);
    static int lge_ws_loop(lua_State *L);
#endif

    // 3D functions
    static int lge_set_3d_camera(lua_State *L);
    static int lge_set_3d_light(lua_State *L); // Avoid using lighting with blue colors due to 3-3-2 color representation limitations
    static int lge_create_3d_model(lua_State *L);
    static int lge_create_3d_instance(lua_State *L);
    static int lge_draw_3d_instance(lua_State *L);

    struct Model3D
    {
        // flat [x1,y1,z1, x2,y2,z2, ...]
        std::vector<float> vertices;
        // 0-based vertex indices, triplets per triangle
        std::vector<uint16_t> indices;
    };

    struct Instance3D
    {
        int modelIndex;                      // index into models3d_
        std::vector<uint16_t> faceColors565; // one color per triangle
    };

    // Camera / projection parameters
    float fov3d_ = 200.0f;
    float camDist3d_ = 100.0f;

    // Registered models & instances
    std::vector<Model3D> models3d_;
    std::vector<Instance3D> instances3d_;

    // Scratch buffers reused every draw (avoid allocations in the hot path)
    std::vector<float> tempVertices3d_;
    std::vector<float> visibleZ_;
    std::vector<int> visibleB1_;
    std::vector<int> visibleB2_;
    std::vector<int> visibleB3_;
    std::vector<uint16_t> visibleColor_;

    // lighting:
    bool lightEnabled_ = false;
    float lightDirX_ = 0.0f;
    float lightDirY_ = 0.0f;
    float lightDirZ_ = -1.0f;   // light coming from camera by default
    float lightAmbient_ = 0.2f; // 0..1
    float lightDiffuse_ = 0.8f; // 0..1

    // End 3D

    // WebSocket
#if ENABLE_WIFI
    WebSocketsClient wsClient_;
    bool wsConnected_ = false;
    int wsCallbackRef_; // Lua registry reference for the message callback
    WiFiInitCallback wifiInitCallback_ = nullptr;
    bool wifiInitialized_ = false;
    static void webSocketEvent(LuaDriver *self, WStype_t type, uint8_t *payload, size_t length);
    void handleWebSocketEvent(WStype_t type, uint8_t *payload, size_t length);
#endif
    // End WebSocket

    static uint16_t parseHexColor(const char *hex);
    static uint16_t scaleColor565(uint16_t c, float factor);

#if DIRTY_RECTS_OPTIMIZATION
    std::vector<DirtyRect> current_dirty_rects_;
    std::vector<DirtyRect> previous_dirty_rects_;
#elif DIRTY_TILE_OPTIMIZATION
    DirtyTileManager *tileManager_;
#endif
    void addDirtyRegion(int x, int y, int w, int h);

    void updateMouseClick();

    static constexpr int TS_MIN_X_CONST = 240;
    static constexpr int TS_MAX_X_CONST = 3780;
    static constexpr int TS_MIN_Y_CONST = 220;
    static constexpr int TS_MAX_Y_CONST = 3800;
    static constexpr const char *RUNTIME_LUA_FILE = "/runtime.luac";
};

#endif // LUA_DRIVER_HPP