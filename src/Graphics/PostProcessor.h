#pragma once
#include <glad/glad.h>
#include <iostream>
#include "Shader.h"

class PostProcessor {
public:
    PostProcessor(int width, int height);
    ~PostProcessor();

    void Resize(int width, int height);
    void Update(float dt);
    void BeginRender();
    void EndRender();

private:
    void InitRenderData();

    Shader screenShader;
    unsigned int MSFBO; // Multisampled Framebuffer (New)
    unsigned int MSRBO; // Multisampled Renderbuffer (New)

    unsigned int FBO;   // Intermediate Framebuffer (Existing)
    unsigned int TCB;   // Texture Color Buffer (Existing)

    unsigned int rectVAO, rectVBO;
    float m_Time;
    int m_Width, m_Height; // Store dimensions for resizing
};