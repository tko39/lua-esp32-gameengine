// --- Add to LuaDriver.cpp ---
#include <Arduino.h>
#include <algorithm> // For std::max and std::min
#include "dirtyRects.hpp"

// Function to calculate the union (bounding box) of two rects
DirtyRect union_rect(const DirtyRect &a, const DirtyRect &b)
{
    DirtyRect result;
    // Find the smallest X and Y coordinates (top-left)
    result.x1 = std::min(a.x1, b.x1);
    result.y1 = std::min(a.y1, b.y1);
    // Find the largest X and Y coordinates (bottom-right)
    result.x2 = std::max(a.x2, b.x2);
    result.y2 = std::max(a.y2, b.y2);
    return result;
}

// Function to check for overlap or adjacency (with a small buffer)
bool are_overlapping_or_adjacent(const DirtyRect &a, const DirtyRect &b)
{
    // Expand both rectangles by 1 pixel in all directions
    int ax1 = a.x1 - 1;
    int ay1 = a.y1 - 1;
    int ax2 = a.x2 + 1;
    int ay2 = a.y2 + 1;
    int bx1 = b.x1 - 1;
    int by1 = b.y1 - 1;
    int bx2 = b.x2 + 1;
    int by2 = b.y2 + 1;

    // Check overlap of expanded rectangles
    bool overlap = !(ax2 < bx1 || ax1 > bx2 || ay2 < by1 || ay1 > by2);

    return overlap;
}

// This function performs the iterative merging of the list
void mergeDirtyRects(std::vector<DirtyRect> &rects)
{
    if (rects.size() <= 1)
        return;

    bool merged_happened_in_this_pass = true;

    // Outer loop repeats the entire merge process until a full pass yields no merges.
    while (merged_happened_in_this_pass)
    {
        merged_happened_in_this_pass = false;
        for (size_t i = 0; i < rects.size(); ++i)
        {
            for (size_t j = i + 1; j < rects.size();)
            {
                if (are_overlapping_or_adjacent(rects[i], rects[j]))
                {
                    rects[i] = union_rect(rects[i], rects[j]);
                    rects[j] = rects.back();
                    rects.pop_back();

                    merged_happened_in_this_pass = true;
                }
                else
                {
                    ++j;
                }
            }
        }
    }
}