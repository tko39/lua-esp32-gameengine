# LuaArduinoEngine (ESP32, PlatformIO)

A PlatformIO/Arduino project for ESP32 for Lua Game Engine [lge](https://luagameengine.com), TFT display rendering (TFT_eSPI), and resistive touch input (XPT2046). Lua scripting is integrated for game logic. Core source is in `/src/` and `/include/`.

## Features

- **TFT display** via [TFT_eSPI](https://github.com/Bodmer/TFT_eSPI)
- **Resistive touch** via [XPT2046_Touchscreen](https://github.com/PaulStoffregen/XPT2046_Touchscreen)
- **Lua scripting** for game logic (see `src/lua_driver.cpp` / `src/lua_driver.hpp`)
- **Single binary**: Arduino-style `setup()` and `loop()`
- **Pin and hardware config** via `platformio.ini` build flags

## Directory Structure

```
/src/               # Main application code (entry: main.cpp, Lua bridge, game logic)
/include/           # Project headers
/platformio.ini     # Build configuration, pin macros, dependencies
/lua/               # Example Lua scripts and applications
/lua/api/           # Lua API documentation
/data/runtime.luac  # Example of compiled runtime
```

## Quick Start

1. **Install [PlatformIO](https://platformio.org/)** (VSCode recommended)
2. **Clone this repo**
3. **Configure pins** in `platformio.ini` (`build_flags` section). Current configuration based on [CYD](https://aliexpress.com/item/1005007883129599.html)
4. **Load Games:**
   To load a 10 examples to the device, use:
   ```sh
   lua/luaToConst.sh -m header lua/bouncingBalls.lua lua/touchSequence.lua lua/sideShooter.lua lua/rotatingBox.lua lua/bouncingRotatingCubes.lua lua/flappyBird.lua lua/3dApiDemo.lua lua/3dApiDemoBounce.lua lua/orbit.lua lua/gemfall.lua
   ```
   This will create an `h` file with the entire code for all 10 examples, once compiled the games will be available.
5. **Build:**
   ```sh
   platformio run
   ```
6. **Upload:**
   ```sh
   platformio run -t upload
   ```
7. **Monitor serial output:**
   ```sh
   platformio device monitor --port COMx --baud 115200
   ```

## Hardware Notes

- **ESP32** (tested with `esp32dev` board, based on [CYD](https://aliexpress.com/item/1005007883129599.html))
- **TFT and touch** share SPI bus; touch uses a separate `SPIClass` instance
- **Pin assignments**: Set in `platformio.ini` (do not hardcode in source)
- **Touch calibration**: Adjust `TS_MIN_X_CONST`, `TS_MAX_X_CONST`, `TS_MIN_Y_CONST`, `TS_MAX_Y_CONST` in `src/luaDriver.cpp` if needed

## Editing & Extending

- **Display/touch/game loop**: Edit `src/main.cpp`
- **Lua integration**: See `src/luaDriver.cpp` and `src/luaDriver.hpp`
- **Pin changes**: Edit `platformio.ini` and rebuild
- **Example Lua scripts**: See the `lua/` directory
- **Lua API documentation**: See the `lua/api/` directory

## Troubleshooting

- **Touch not working?** Check pin macros and calibration constants
- **Display issues?** Confirm SPI wiring and `platformio.ini` settings

## License

See [LICENSE](LICENSE).
