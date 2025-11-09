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
/lua/               # Example lua applications, color table, and 32 bit lua compiler (for windows)
/data/runtime.luac  # Example of compiled runtime
```

## Quick Start

1. **Install [PlatformIO](https://platformio.org/)** (VSCode recommended)
2. **Clone this repo**
3. **Configure pins** in `platformio.ini` (`build_flags` section). Current configuration based on [CYD](https://aliexpress.com/item/1005007883129599.html)
4. **Build:**
   ```sh
   platformio run
   ```
5. **Upload:**
   ```sh
   platformio run -t upload
   ```
6. **Monitor serial output:**
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

## Troubleshooting

- **Touch not working?** Check pin macros and calibration constants
- **Display issues?** Confirm SPI wiring and `platformio.ini` settings
- **See `.github/copilot-instructions.md`** for detailed project conventions and patterns

## License

See [LICENSE](LICENSE).
