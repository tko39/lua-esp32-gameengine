#pragma once
#ifndef FLAGS_H
#define FLAGS_H

// Definitions
#define USE_VSPI

// Debugging
#define DEBUG_DELAY_AVERAGE_PRINT 0
#define DEBUG_SHOW_AVAILABLE_COLORS 0
#define DEBUG_PROFILING 1

// Graphics optimizations 1 - Tile-based dirty rectangle management, 2 - Simple dirty rectangle list
// Dirty tiles are better for more static scenes, or with large areas of changes
// Dirty rects are better for highly dynamic scenes with many sparse changes
#define GRAPHICS_OPTIMIZATIONS 1

// Performance improvements
#if GRAPHICS_OPTIMIZATIONS == 1
#define DIRTY_RECTS_OPTIMIZATION 0
#define DIRTY_TILE_OPTIMIZATION 1
#elif GRAPHICS_OPTIMIZATIONS == 2
#define DIRTY_RECTS_OPTIMIZATION 1
#define DIRTY_TILE_OPTIMIZATION 0
#endif

// Features
#define ENABLE_BLE 0
#define ENABLE_WIFI 1 // Takes a lot of memory, in particular causes fragmentation. Lowers the capabilities of the Lua scripts.
#define LUA_FROM_FILE 0

#endif // FLAGS_H