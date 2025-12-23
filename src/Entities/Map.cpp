#include "Map.h"
#include <cmath>
#include <iostream>

Map::Map() {
    // 1 = Wall, 0 = Path
    // Hardcoded layout based on your previous "Loopy" maze
    int tempGrid[MAP_HEIGHT][MAP_WIDTH] = {
        {1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
        {1, 0, 0, 0, 1, 0, 0, 0, 0, 1},
        {1, 0, 1, 0, 1, 0, 1, 1, 0, 1},
        {1, 0, 1, 0, 0, 0, 0, 0, 0, 1},
        {1, 0, 1, 1, 1, 1, 1, 1, 0, 1},
        {1, 0, 0, 0, 0, 0, 0, 1, 0, 1},
        {1, 1, 1, 1, 0, 1, 0, 1, 0, 1},
        {1, 0, 0, 0, 0, 1, 0, 0, 0, 1},
        {1, 0, 1, 1, 1, 1, 1, 1, 0, 1},
        {1, 1, 1, 1, 1, 1, 1, 1, 1, 1}
    };

    // Copy to member variable
    for(int z = 0; z < MAP_HEIGHT; z++) {
        for(int x = 0; x < MAP_WIDTH; x++) {
            grid[z][x] = tempGrid[z][x];
        }
    }
}

int Map::GetTile(int x, int z) const {
    if (x < 0 || x >= MAP_WIDTH || z < 0 || z >= MAP_HEIGHT) return 1; // Out of bounds is a wall
    return grid[z][x];
}

bool Map::IsWall(float x, float z, float radius) const {
    // 1. Identify which grid cell the center of the player is in
    int gridX = static_cast<int>(std::round(x));
    int gridZ = static_cast<int>(std::round(z));

    // 2. Bounds check (if outside map, it's a wall)
    if (gridX < 0 || gridX >= MAP_WIDTH || gridZ < 0 || gridZ >= MAP_HEIGHT) return true;

    // 3. If the grid cell itself is a wall, calculate precise AABB overlap
    if (grid[gridZ][gridX] == 1) {
        // Wall boundaries (walls are 1x1 centered on integer coordinates)
        float wallMinX = static_cast<float>(gridX) - 0.5f;
        float wallMaxX = static_cast<float>(gridX) + 0.5f;
        float wallMinZ = static_cast<float>(gridZ) - 0.5f;
        float wallMaxZ = static_cast<float>(gridZ) + 0.5f;

        // Player boundaries
        float pMinX = x - radius;
        float pMaxX = x + radius;
        float pMinZ = z - radius;
        float pMaxZ = z + radius;

        // Check for AABB intersection
        bool collisionX = pMaxX > wallMinX && pMinX < wallMaxX;
        bool collisionZ = pMaxZ > wallMinZ && pMinZ < wallMaxZ;

        return collisionX && collisionZ;
    }

    return false;
}