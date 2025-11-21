#pragma once
#ifndef FLAGS_H
#define FLAGS_H

// Definitions
#define USE_VSPI

// Debugging
#define DEBUG_DELAY_AVERAGE_PRINT 0
#define DEBUG_SHOW_AVAILABLE_COLORS 0
#define DEBUG_PROFILING 1

// Performance improvements
#define DIRTY_RECTS_OPTIMIZATION 1

// Features
#define ENABLE_BLE 0
#define ENABLE_WIFI 1 // Takes a lot of memory, in particular causes fragmentation. Lowers the capabilities of the Lua scripts.
#define LUA_FROM_FILE 0

#endif // FLAGS_H