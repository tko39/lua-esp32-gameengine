#pragma once
#ifndef LUA_DRIVER_HPP
#define LUA_DRIVER_HPP

// Forward-declare Lua state type to avoid including Lua headers in this header.
struct lua_State;

class LuaDriver
{
public:
    // Lifecycle
    LuaDriver();
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
    char lua_code_[256];

    void registerFunctions();
};

// C-style wrappers kept for compatibility with existing code
void setup_lua();
void loop_lua();

#endif // LUA_DRIVER_HPP