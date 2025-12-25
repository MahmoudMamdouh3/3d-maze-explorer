#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>
#include "Shader.h"

class Renderer {
public:
    Renderer();
    ~Renderer();


    void DrawCube(Shader& shader, const glm::mat4& model, unsigned int textureID);



    void SetupInstancedWalls(const std::vector<glm::mat4>& transforms);
    void SetupInstancedFloors(const std::vector<glm::mat4>& transforms);


    void DrawInstancedWalls(Shader& instancedShader, unsigned int textureID, int count);
    void DrawInstancedFloors(Shader& instancedShader, unsigned int textureID, int count);

private:
    unsigned int cubeVAO, cubeVBO;


    unsigned int wallInstanceVBO;
    unsigned int floorInstanceVBO;

    void InitCubeMesh();
};