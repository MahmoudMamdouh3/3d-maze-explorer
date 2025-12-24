#pragma once
#include <vector>
#include <string>
#include <glm/glm.hpp>
#include "../Physics/AABB.h"

// Result of a "look" check
struct RaycastResult {
    bool hit;           // Did we hit anything?
    int tileX, tileZ;   // Which grid cell?
    int tileType;       // What is it? (1=Wall, 2=Door, etc)
    float distance;     // How far away?
};

class Map {
public:
    Map();

    bool LoadLevel(const std::string& path, glm::vec3& outPlayerStart, glm::vec3& outPaperPos);
    std::vector<AABB> GetNearbyWalls(glm::vec3 position, float range) const;

    int GetTile(int x, int z) const;
    void SetTile(int x, int z, int type); // NEW: Allow changing the map (Opening doors)

    // NEW: The "Eye" Function
    RaycastResult CastRay(glm::vec3 start, glm::vec3 direction, float maxDistance) const;

    int GetWidth() const { return m_Width; }
    int GetHeight() const { return m_Height; }

private:
    int m_Width;
    int m_Height;
    std::vector<int> m_Grid;
};