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

    void BeginRender();
    void EndRender();

private:
    void InitRenderData();

    Shader screenShader;


    unsigned int MSFBO;
    unsigned int RBO;
    unsigned int DB;


    unsigned int FBO;
    unsigned int TCB;

    unsigned int rectVAO, rectVBO;
    float m_Time;
    int m_Width, m_Height;
};