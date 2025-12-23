#pragma once
#include <vector>

// Constants for Map Size
constexpr int MAP_WIDTH = 10;
constexpr int MAP_HEIGHT = 10;

class Map {
public:
    Map();

    // Returns true if there is a wall at world coordinates (x, z)
    // Radius is used to prevent the player from clipping into the wall
    bool IsWall(float x, float z, float playerRadius) const;

    // Get the raw grid data (for rendering)
    int GetTile(int x, int z) const;

    int GetWidth() const { return MAP_WIDTH; }
    int GetHeight() const { return MAP_HEIGHT; }

private:
    int grid[MAP_HEIGHT][MAP_WIDTH];
};