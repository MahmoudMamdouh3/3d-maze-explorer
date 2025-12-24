#pragma once
#include <glad/glad.h>
#include <iostream>
#include <vector>
#include "Shader.h"

class PostProcessor {
public:
    PostProcessor(int width, int height);
    ~PostProcessor();

    void Resize(int width, int height);
    void Update(float dt);

    void BeginRender(); // Binds the MSAA buffer
    void EndRender();   // Resolves MSAA -> Texture -> Screen

private:
    void InitRenderData();

    Shader screenShader;

    // 1. MSAA Buffer (We render the 3D scene here first)
    unsigned int MSFBO; // Multisampled Framebuffer
    unsigned int RBO;   // Multisampled Color Renderbuffer
    unsigned int DB;    // Multisampled Depth Renderbuffer

    // 2. Intermediate Buffer (We copy the result here to make it a texture)
    unsigned int FBO;   // Regular Framebuffer
    unsigned int TCB;   // Texture Color Buffer (The final image)

    unsigned int rectVAO, rectVBO;
    float m_Time;
    int m_Width, m_Height;
};