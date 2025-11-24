#include "dirtyTiles.hpp"
#include <algorithm>
#include <cstring>

DirtyTileManager::DirtyTileManager(int screenWidth, int screenHeight)
    : screenWidth_(screenWidth), screenHeight_(screenHeight), spriteBuffer_(nullptr), spriteWidth_(0)
{
    // Calculate number of tiles needed (round up)
    tilesX_ = (screenWidth + TILE_SIZE - 1) / TILE_SIZE;
    tilesY_ = (screenHeight + TILE_SIZE - 1) / TILE_SIZE;

    // Allocate tile buffers
    int totalTiles = tilesX_ * tilesY_;
    currentTiles_.resize(totalTiles, false);
    previousTiles_.resize(totalTiles, false);
    tileHashes_.resize(totalTiles, 0);
}

void DirtyTileManager::setSpriteBuffer(uint8_t *buffer, int width)
{
    spriteBuffer_ = buffer;
    spriteWidth_ = width;
}

void DirtyTileManager::markDirtyRegion(int x, int y, int w, int h)
{
    if (w <= 0 || h <= 0)
        return;

    // Clamp to screen bounds
    int x1 = std::max(0, x);
    int y1 = std::max(0, y);
    int x2 = std::min(screenWidth_ - 1, x + w - 1);
    int y2 = std::min(screenHeight_ - 1, y + h - 1);

    if (x1 > x2 || y1 > y2)
        return;

    // Convert pixel coordinates to tile coordinates
    int tileX1 = x1 / TILE_SIZE;
    int tileY1 = y1 / TILE_SIZE;
    int tileX2 = x2 / TILE_SIZE;
    int tileY2 = y2 / TILE_SIZE;

    // Mark all tiles in the region as dirty
    for (int ty = tileY1; ty <= tileY2; ++ty)
    {
        for (int tx = tileX1; tx <= tileX2; ++tx)
        {
            markTile(tx, ty);
        }
    }
}

std::vector<TileRect> DirtyTileManager::getUpdateRectangles()
{
    std::vector<TileRect> rects;

    // Create a working copy that includes both current and previous tiles
    std::vector<bool> workingTiles(tilesX_ * tilesY_, false);

    for (int i = 0; i < tilesX_ * tilesY_; ++i)
    {
        workingTiles[i] = currentTiles_[i] || previousTiles_[i];
    }

    // Combine tiles into rectangles using a greedy algorithm
    combineTilesIntoRectangles(rects);

    return rects;
}

void DirtyTileManager::combineTilesIntoRectangles(std::vector<TileRect> &rects)
{
    // Create a visited array
    std::vector<bool> visited(tilesX_ * tilesY_, false);

    // Scan through all tiles
    for (int ty = 0; ty < tilesY_; ++ty)
    {
        for (int tx = 0; tx < tilesX_; ++tx)
        {
            int idx = getTileIndex(tx, ty);

            // Skip if already visited or not dirty
            if (visited[idx] || !isTileDirty(tx, ty))
                continue;

            // Check if tile content actually changed
            uint32_t currentHash = computeTileHash(tx, ty);
            if (currentHash == tileHashes_[idx])
            {
                // Content unchanged, skip this tile
                visited[idx] = true;
                continue;
            }

            // Update stored hash
            tileHashes_[idx] = currentHash;

            // Start a new rectangle at this tile
            int rectX1 = tx;
            int rectY1 = ty;
            int rectX2 = tx;
            int rectY2 = ty;

            // Try to expand horizontally first
            while (rectX2 + 1 < tilesX_ &&
                   isTileDirty(rectX2 + 1, ty) &&
                   !visited[getTileIndex(rectX2 + 1, ty)])
            {
                int nextIdx = getTileIndex(rectX2 + 1, ty);
                uint32_t nextHash = computeTileHash(rectX2 + 1, ty);

                // Only expand if content actually changed
                if (nextHash == tileHashes_[nextIdx])
                {
                    visited[nextIdx] = true;
                    break;
                }

                tileHashes_[nextIdx] = nextHash;
                rectX2++;
            }

            // Try to expand vertically (checking the entire width)
            bool canExpandVertically = true;
            while (canExpandVertically && rectY2 + 1 < tilesY_)
            {
                // Check if all tiles in the next row are dirty, unvisited, and content changed
                for (int x = rectX1; x <= rectX2; ++x)
                {
                    int nextIdx = getTileIndex(x, rectY2 + 1);
                    if (!isTileDirty(x, rectY2 + 1) || visited[nextIdx])
                    {
                        canExpandVertically = false;
                        break;
                    }

                    // Check if content actually changed
                    uint32_t nextHash = computeTileHash(x, rectY2 + 1);
                    if (nextHash == tileHashes_[nextIdx])
                    {
                        // Mark as visited but don't expand
                        visited[nextIdx] = true;
                        canExpandVertically = false;
                        break;
                    }
                }

                if (canExpandVertically)
                {
                    // Update hashes for the new row
                    for (int x = rectX1; x <= rectX2; ++x)
                    {
                        int nextIdx = getTileIndex(x, rectY2 + 1);
                        tileHashes_[nextIdx] = computeTileHash(x, rectY2 + 1);
                    }
                    rectY2++;
                }
            }

            // Mark all tiles in this rectangle as visited
            for (int y = rectY1; y <= rectY2; ++y)
            {
                for (int x = rectX1; x <= rectX2; ++x)
                {
                    visited[getTileIndex(x, y)] = true;
                }
            }

            // Convert tile coordinates to pixel coordinates
            TileRect rect;
            rect.x1 = rectX1 * TILE_SIZE;
            rect.y1 = rectY1 * TILE_SIZE;
            rect.x2 = std::min((rectX2 + 1) * TILE_SIZE - 1, screenWidth_ - 1);
            rect.y2 = std::min((rectY2 + 1) * TILE_SIZE - 1, screenHeight_ - 1);

            rects.push_back(rect);
        }
    }
}

void DirtyTileManager::swapBuffers()
{
    // Move current to previous
    previousTiles_.swap(currentTiles_);

    // Clear current
    std::fill(currentTiles_.begin(), currentTiles_.end(), false);
}

void DirtyTileManager::clear()
{
    std::fill(currentTiles_.begin(), currentTiles_.end(), false);
    std::fill(previousTiles_.begin(), previousTiles_.end(), false);
}

uint32_t DirtyTileManager::computeTileHash(int tileX, int tileY) const
{
    if (!spriteBuffer_)
        return 0;

    // Calculate pixel bounds for this tile
    int x1 = tileX * TILE_SIZE;
    int y1 = tileY * TILE_SIZE;
    int x2 = std::min((tileX + 1) * TILE_SIZE, screenWidth_);
    int y2 = std::min((tileY + 1) * TILE_SIZE, screenHeight_);

    // Simple FNV-1a hash
    uint32_t hash = 2166136261u;

    for (int y = y1; y < y2; ++y)
    {
        for (int x = x1; x < x2; ++x)
        {
            uint8_t pixel = spriteBuffer_[y * spriteWidth_ + x];
            hash ^= pixel;
            hash *= 16777619u;
        }
    }

    return hash;
}
