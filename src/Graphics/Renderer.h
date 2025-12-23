#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include "Shader.h"

class Renderer {
public:
    Renderer();
    ~Renderer();

    // Renders a textured cube at a specific location/size
    void DrawCube(Shader& shader, const glm::mat4& model, unsigned int textureID);

private:
    unsigned int VAO, VBO;
    void InitCubeMesh();
};