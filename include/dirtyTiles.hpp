#pragma once
#include <vector>
#include <cstdint>

// Tile size in pixels (adjustable based on performance)
constexpr int TILE_SIZE = 16;

struct DirtyTile
{
    uint8_t tileX, tileY; // Tile coordinates (not pixel coordinates)
};

struct TileRect
{
    int x1, y1, x2, y2; // Pixel coordinates of the bounding box
};

class DirtyTileManager
{
public:
    DirtyTileManager(int screenWidth, int screenHeight);

    // Set the sprite buffer to compute tile hashes
    void setSpriteBuffer(uint8_t *buffer, int width);

    // Mark a pixel region as dirty
    void markDirtyRegion(int x, int y, int w, int h);

    // Get optimized rectangles for updating (combines adjacent tiles)
    std::vector<TileRect> getUpdateRectangles();

    // Clear current dirty tiles and swap with previous
    void swapBuffers();

    // Clear all tiles
    void clear();

private:
    int screenWidth_;
    int screenHeight_;
    int tilesX_;
    int tilesY_;

    // Sprite buffer for content comparison
    uint8_t *spriteBuffer_;
    int spriteWidth_;

    // Bitset for current and previous frame dirty tiles
    std::vector<bool> currentTiles_;
    std::vector<bool> previousTiles_;

    // Hash of tile content (for detecting actual changes)
    std::vector<uint32_t> tileHashes_;

    // Helper to get tile index from tile coordinates
    inline int getTileIndex(int tileX, int tileY) const
    {
        return tileY * tilesX_ + tileX;
    }

    // Helper to mark a single tile as dirty
    inline void markTile(int tileX, int tileY)
    {
        if (tileX >= 0 && tileX < tilesX_ && tileY >= 0 && tileY < tilesY_)
        {
            currentTiles_[getTileIndex(tileX, tileY)] = true;
        }
    }

    // Check if a tile is dirty (in current or previous frame)
    inline bool isTileDirty(int tileX, int tileY) const
    {
        if (tileX < 0 || tileX >= tilesX_ || tileY < 0 || tileY >= tilesY_)
            return false;

        int idx = getTileIndex(tileX, tileY);
        return currentTiles_[idx] || previousTiles_[idx];
    }

    // Combine adjacent dirty tiles into rectangles
    void combineTilesIntoRectangles(std::vector<TileRect> &rects);

    // Compute hash of a tile's content
    uint32_t computeTileHash(int tileX, int tileY) const;
};
