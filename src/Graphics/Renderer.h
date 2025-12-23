#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>
#include "Shader.h"

class Renderer {
public:
    Renderer();
    ~Renderer();

    // The Old Way (Keep for single objects like Player/Paper)
    void DrawCube(Shader& shader, const glm::mat4& model, unsigned int textureID);

    // The AAA Way (Instanced Rendering)
    // Call this ONCE at startup to send data to GPU
    void SetupInstancedWalls(const std::vector<glm::mat4>& transforms);
    void SetupInstancedFloors(const std::vector<glm::mat4>& transforms);

    // Call this EVERY FRAME (Draws thousands in 1 call)
    void DrawInstancedWalls(Shader& instancedShader, unsigned int textureID, int count);
    void DrawInstancedFloors(Shader& instancedShader, unsigned int textureID, int count);

private:
    unsigned int cubeVAO, cubeVBO;

    // Instancing Buffers
    unsigned int wallInstanceVBO;
    unsigned int floorInstanceVBO;

    void InitCubeMesh();
};