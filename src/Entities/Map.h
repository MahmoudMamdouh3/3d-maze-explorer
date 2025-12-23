#pragma once
#include <vector>
#include <string>
#include <glm/glm.hpp>
#include "../Physics/AABB.h"

class Map {
public:
    Map();

    // Load level from a text file
    // Returns the Player Start position found in the file
    bool LoadLevel(const std::string& path, glm::vec3& outPlayerStart, glm::vec3& outPaperPos);

    std::vector<AABB> GetNearbyWalls(glm::vec3 position, float range) const;

    int GetTile(int x, int z) const;
    int GetWidth() const { return m_Width; }
    int GetHeight() const { return m_Height; }

private:
    int m_Width;
    int m_Height;
    std::vector<int> m_Grid; // Dynamic array (vector) instead of [10][10]
};