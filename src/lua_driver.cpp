#include <Arduino.h>
#include "lua.hpp"

// Lua interpreter instance
static lua_State *L;

// LED control hook
static int lua_led_control(lua_State *L)
{
    int pin = luaL_checkinteger(L, 1);   // Get first argument
    int state = luaL_checkinteger(L, 2); // Get second argument

    pinMode(pin, OUTPUT);
    digitalWrite(pin, state);

    Serial.printf("Lua: Set LED pin %d to %d\n", pin, state);
    return 0; // No return values
}

// Read analog pin hook
static int lua_analog_read(lua_State *L)
{
    int pin = luaL_checkinteger(L, 1);
    int value = analogRead(pin);

    lua_pushinteger(L, value); // Return value to Lua
    Serial.printf("Lua: Read analog pin %d: %d\n", pin, value);
    return 1; // One return value
}

// Delay with milliseconds hook
static int lua_delay_ms(lua_State *L)
{
    int ms = luaL_checkinteger(L, 1);
    delay(ms);
    return 0;
}

// Get system time hook
static int lua_millis(lua_State *L)
{
    lua_pushinteger(L, millis());
    return 1;
}

// Print to Serial from Lua
static int lua_print(lua_State *L)
{
    const char *str = luaL_checkstring(L, 1);
    Serial.print("Lua print: ");
    Serial.println(str);
    return 0;
}

// Register all C functions to Lua
void register_lua_functions(lua_State *L)
{
    lua_register(L, "led", lua_led_control);
    lua_register(L, "analog_read", lua_analog_read);
    lua_register(L, "delay_ms", lua_delay_ms);
    lua_register(L, "millis", lua_millis);
    lua_register(L, "print", lua_print);
}

static int lua_get_system_info(lua_State *L)
{
    // Create a Lua table
    lua_createtable(L, 0, 4);

    // Add table fields
    lua_pushstring(L, "ESP32");
    lua_setfield(L, -2, "chip");

    lua_pushinteger(L, ESP.getFreeHeap());
    lua_setfield(L, -2, "free_heap");

    lua_pushinteger(L, ESP.getCpuFreqMHz());
    lua_setfield(L, -2, "cpu_freq");

    lua_pushinteger(L, millis());
    lua_setfield(L, -2, "uptime");

    return 1; // Return the table
}

// Call Lua function from C++
void call_lua_function_from_cpp()
{
    lua_getglobal(L, "lua_callback"); // Get Lua function
    if (lua_isfunction(L, -1))
    {
        lua_pushinteger(L, 42); // Push argument
        if (lua_pcall(L, 1, 0, 0) != LUA_OK)
        {
            Serial.print("Callback error: ");
            Serial.println(lua_tostring(L, -1));
        }
    }
    lua_pop(L, 1); // Clean stack
}

void setup_lua()
{
    Serial.begin(115200);
    delay(1000);

    Serial.println("ESP32 Lua Interpreter Starting...");

    // Initialize Lua
    L = luaL_newstate();
    luaL_openlibs(L); // Open standard libraries

    // Register our custom functions
    register_lua_functions(L);

    // Lua script with custom functions
    const char *lua_script = R"(
    print("Lua script started!")

    -- Blink LED on GPIO 16
    local led_pin = 16
    
    for i = 1, 5 do
        print("Blink " .. i .. " at time: " .. millis())
        led(led_pin, 1)  -- ON
        delay_ms(500)
        led(led_pin, 0)  -- OFF
        delay_ms(500)
    end
    
    -- Read analog values
    local analog_value = analog_read(34)
    print("Analog pin 34 value: " .. analog_value)
    
    -- Conditional logic based on sensor reading
    if analog_value > 2000 then
        print("High light level detected!")
    else
        print("Low light level")
    end
    
    print("Script completed!")
  )";

    // Execute Lua script
    int result = luaL_dostring(L, lua_script);
    if (result != LUA_OK)
    {
        const char *error = lua_tostring(L, -1);
        Serial.print("Lua Error: ");
        Serial.println(error);
    }
}

static int led_status = 0;
static char lua_code[128];

void loop_lua()
{
    snprintf(lua_code, sizeof(lua_code), R"(
        local led_pin = 16
        print("Blink at time: " .. millis())
        led(led_pin, %d)  -- ON
    )",
             led_status);
    led_status = 1 - led_status; // Toggle status

    int result = luaL_dostring(L, lua_code);
    if (result != LUA_OK)
    {
        Serial.println("Error in periodic script");
    }

    delay(1000);
}