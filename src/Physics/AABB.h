#pragma once
#include <glm/glm.hpp>
#include <algorithm>

struct AABB {
    glm::vec3 min;
    glm::vec3 max;


    AABB(glm::vec3 position, glm::vec3 size) {

        glm::vec3 halfSize = size * 0.5f;
        min = position - halfSize;
        max = position + halfSize;
    }


    bool Intersects(const AABB& other) const {
        return (min.x <= other.max.x && max.x >= other.min.x) &&
               (min.y <= other.max.y && max.y >= other.min.y) &&
               (min.z <= other.max.z && max.z >= other.min.z);
    }


    glm::vec3 GetPenetration(const AABB& other) const {
        glm::vec3 penetration(0.0f);

        float xOverlap = std::min(max.x, other.max.x) - std::max(min.x, other.min.x);
        float yOverlap = std::min(max.y, other.max.y) - std::max(min.y, other.min.y);
        float zOverlap = std::min(max.z, other.max.z) - std::max(min.z, other.min.z);

        if (xOverlap > 0 && yOverlap > 0 && zOverlap > 0) {

            if (xOverlap < yOverlap && xOverlap < zOverlap) penetration.x = xOverlap;
            else if (yOverlap < zOverlap) penetration.y = yOverlap;
            else penetration.z = zOverlap;
        }

        return penetration;
    }
};