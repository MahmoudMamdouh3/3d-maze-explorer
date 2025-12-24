#include "Map.h"
#include <fstream>
#include <iostream>
#include <algorithm>
#include <cmath> // Added for round()

Map::Map() : m_Width(0), m_Height(0) {}

bool Map::LoadLevel(const std::string& path, glm::vec3& outPlayerStart, glm::vec3& outPaperPos) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "CRITICAL: Failed to load level: " << path << std::endl;
        return false;
    }

    std::string line;
    std::vector<std::string> lines;

    // 1. Read all lines
    while (std::getline(file, line)) {
        if (!line.empty()) lines.push_back(line);
    }

    if (lines.empty()) return false;

    // 2. Set Dimensions
    m_Height = lines.size();
    m_Width = lines[0].size();
    m_Grid.resize(m_Width * m_Height);

    // 3. Parse Symbols
    for (int z = 0; z < m_Height; z++) {
        for (int x = 0; x < m_Width; x++) {
            char tile = lines[z][x];
            int index = z * m_Width + x;

            if (tile == '#') {
                m_Grid[index] = 1; // Wall
            }
            else if (tile == 'D') {
                m_Grid[index] = 2; // Normal Door (Closed)
            }
            else if (tile == 'K') {
                m_Grid[index] = 4; // Red Keycard (Collectible) - NEW
            }
            else if (tile == 'L') {
                m_Grid[index] = 5; // Locked Door (Needs Key) - NEW
            }
            else if (tile == 'P') {
                m_Grid[index] = 0; // Floor
                outPlayerStart = glm::vec3(x + 0.5f, 0.0f, z + 0.5f);
            }
            else if (tile == 'O') {
                m_Grid[index] = 0; // Floor
                outPaperPos = glm::vec3(x + 0.5f, 0.5f, z + 0.5f);
            }
            else {
                m_Grid[index] = 0; // Floor (.)
            }
        }
    }

    std::cout << "Level Loaded: " << m_Width << "x" << m_Height << std::endl;
    return true;
}

int Map::GetTile(int x, int z) const {
    if (x < 0 || x >= m_Width || z < 0 || z >= m_Height) return 1; // Return wall if out of bounds
    return m_Grid[z * m_Width + x];
}

void Map::SetTile(int x, int z, int type) {
    if (x >= 0 && x < m_Width && z >= 0 && z < m_Height) {
        m_Grid[z * m_Width + x] = type;
    }
}

// UPDATED COLLISION LOGIC
std::vector<AABB> Map::GetNearbyWalls(glm::vec3 position, float range) const {
    std::vector<AABB> walls;

    // Expand search radius slightly to ensure we don't miss corners
    int startX = std::max(0, static_cast<int>(position.x - range - 1.0f));
    int endX = std::min(m_Width - 1, static_cast<int>(position.x + range + 1.0f));
    int startZ = std::max(0, static_cast<int>(position.z - range - 1.0f));
    int endZ = std::min(m_Height - 1, static_cast<int>(position.z + range + 1.0f));

    for (int z = startZ; z <= endZ; z++) {
        for (int x = startX; x <= endX; x++) {
            int tile = GetTile(x, z);
            // 1=Wall, 2=Door, 5=LockedDoor
            if (tile == 1 || tile == 2 || tile == 5) {
                // FIX: Add +0.5f to X and Z to center the collider in the grid cell
                walls.emplace_back(
                    glm::vec3(x + 0.5f, 1.5f, z + 0.5f),
                    glm::vec3(1.0f, 4.0f, 1.0f)
                );
            }
        }
    }
    return walls;
}

RaycastResult Map::CastRay(glm::vec3 start, glm::vec3 direction, float maxDistance) const {
    RaycastResult result = {false, 0, 0, 0, 0.0f};

    float stepSize = 0.1f;
    glm::vec3 currentPos = start;
    float traveled = 0.0f;

    while (traveled < maxDistance) {
        currentPos += direction * stepSize;
        traveled += stepSize;

        int gridX = static_cast<int>(std::round(currentPos.x));
        int gridZ = static_cast<int>(std::round(currentPos.z));

        if (gridX < 0 || gridX >= m_Width || gridZ < 0 || gridZ >= m_Height) {
            continue;
        }

        int tile = GetTile(gridX, gridZ);

        // Hit logic: Stop at Wall(1), Door(2), or Locked Door(5)
        if (tile == 1 || tile == 2 || tile == 5) {
            result.hit = true;
            result.tileX = gridX;
            result.tileZ = gridZ;
            result.tileType = tile;
            result.distance = traveled;
            return result;
        }
    }

    return result;
}