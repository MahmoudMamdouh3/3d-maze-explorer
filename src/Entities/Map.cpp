#include "Map.h"
#include <fstream>
#include <iostream>
#include <algorithm>

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
            } else if (tile == 'P') {
                m_Grid[index] = 0; // Floor
                outPlayerStart = glm::vec3(x + 0.5f, 0.0f, z + 0.5f); // Center of tile
            } else if (tile == 'O') {
                m_Grid[index] = 0; // Floor
                outPaperPos = glm::vec3(x + 0.5f, 0.5f, z + 0.5f);
            } else {
                m_Grid[index] = 0; // Floor (.)
            }
        }
    }

    std::cout << "Level Loaded: " << m_Width << "x" << m_Height << std::endl;
    return true;
}

int Map::GetTile(int x, int z) const {
    if (x < 0 || x >= m_Width || z < 0 || z >= m_Height) return 1;
    return m_Grid[z * m_Width + x];
}

std::vector<AABB> Map::GetNearbyWalls(glm::vec3 position, float range) const {
    std::vector<AABB> walls;

    int startX = std::max(0, static_cast<int>(position.x - range));
    int endX = std::min(m_Width - 1, static_cast<int>(position.x + range));
    int startZ = std::max(0, static_cast<int>(position.z - range));
    int endZ = std::min(m_Height - 1, static_cast<int>(position.z + range));

    for (int z = startZ; z <= endZ; z++) {
        for (int x = startX; x <= endX; x++) {
            if (GetTile(x, z) == 1) {
                walls.emplace_back(glm::vec3(x, 1.5f, z), glm::vec3(1.0f, 4.0f, 1.0f));
            }
        }
    }
    return walls;
}