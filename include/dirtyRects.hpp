#pragma once
#include <vector>
struct DirtyRect
{
    int x1, y1, x2, y2; // Coordinates of the bounding box
};

void mergeDirtyRects(std::vector<DirtyRect> &rects);