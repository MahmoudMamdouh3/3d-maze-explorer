#pragma once
#include <glad/glad.h>
#include <iostream>
#include "Shader.h"

class PostProcessor {
public:
    PostProcessor(int width, int height);
    ~PostProcessor();

    // Call this BEFORE rendering the 3D game
    void BeginRender();

    // Call this AFTER rendering the 3D game (draws the screen quad)
    void EndRender();

    // Update screen size if window resizes
    void Resize(int width, int height);

    // Update time for grain animation
    void Update(float dt);

private:
    unsigned int FBO;       // Framebuffer Object
    unsigned int TCB;       // Texture Color Buffer
    unsigned int RBO;       // Render Buffer Object (Depth)
    unsigned int rectVAO, rectVBO;

    Shader screenShader;
    float m_Time;

    void InitRenderData();
};