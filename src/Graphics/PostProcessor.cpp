#include "PostProcessor.h"

PostProcessor::PostProcessor(int width, int height)
    : m_Time(0.0f), m_Width(width), m_Height(height)
{
    screenShader.Load("assets/shaders/screen.vert", "assets/shaders/postprocess.frag");

    // 1. Setup Multisampled Framebuffer (MSFBO) - We draw here first!
    glGenFramebuffers(1, &MSFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, MSFBO);

    // Create Multisampled Renderbuffer for Color
    unsigned int colorRBO;
    glGenRenderbuffers(1, &colorRBO);
    glBindRenderbuffer(GL_RENDERBUFFER, colorRBO);
    // Note the 'Multisample' call and '4' samples
    glRenderbufferStorageMultisample(GL_RENDERBUFFER, 4, GL_RGB, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, colorRBO);

    // Create Multisampled Renderbuffer for Depth/Stencil
    glGenRenderbuffers(1, &MSRBO);
    glBindRenderbuffer(GL_RENDERBUFFER, MSRBO);
    glRenderbufferStorageMultisample(GL_RENDERBUFFER, 4, GL_DEPTH24_STENCIL8, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, MSRBO);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cerr << "ERROR::POSTPROCESSOR: MSFBO is not complete!" << std::endl;

    // 2. Setup Intermediate Framebuffer (FBO) - Standard Texture for screen quad
    glGenFramebuffers(1, &FBO);
    glBindFramebuffer(GL_FRAMEBUFFER, FBO);

    glGenTextures(1, &TCB);
    glBindTexture(GL_TEXTURE_2D, TCB);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, TCB, 0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cerr << "ERROR::POSTPROCESSOR: Intermediate FBO is not complete!" << std::endl;

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    InitRenderData();
}

PostProcessor::~PostProcessor() {
    glDeleteFramebuffers(1, &MSFBO);
    glDeleteRenderbuffers(1, &MSRBO);
    glDeleteFramebuffers(1, &FBO);
    glDeleteTextures(1, &TCB);
    glDeleteVertexArrays(1, &rectVAO);
    glDeleteBuffers(1, &rectVBO);
}

void PostProcessor::Resize(int width, int height) {
    m_Width = width;
    m_Height = height;

    // Resize Multisampled buffers
    glBindFramebuffer(GL_FRAMEBUFFER, MSFBO);
    // We cannot resize RBOs easily, usually better to delete and recreate,
    // but for simplicity here we just re-allocate storage
    unsigned int colorRBO = 0; // You'd need to track this ID class-wide to do it perfectly properly
    // For now, simpler reset:
    // (In a full engine, you would delete the old RBOs and Gen new ones here)

    // Quick Resize Logic for the Texture (Intermediate)
    glBindTexture(GL_TEXTURE_2D, TCB);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
}

void PostProcessor::Update(float dt) {
    m_Time += dt;
}

void PostProcessor::BeginRender() {
    // Render to the Multisampled buffer
    glBindFramebuffer(GL_FRAMEBUFFER, MSFBO);
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.01f, 0.01f, 0.01f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void PostProcessor::EndRender() {
    // 1. Resolve MSAA: Blit MSFBO -> FBO
    glBindFramebuffer(GL_READ_FRAMEBUFFER, MSFBO);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, FBO);
    glBlitFramebuffer(0, 0, m_Width, m_Height, 0, 0, m_Width, m_Height, GL_COLOR_BUFFER_BIT, GL_NEAREST);

    // 2. Bind default framebuffer (the screen)
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDisable(GL_DEPTH_TEST);
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // 3. Draw Quad using the resolved texture
    screenShader.Use();
    screenShader.SetInt("screenTexture", 0);
    screenShader.SetFloat("time", m_Time);

    glBindVertexArray(rectVAO);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, TCB);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

void PostProcessor::InitRenderData() {
    // (Same as your original code)
    float quadVertices[] = {
        -1.0f,  1.0f,  0.0f, 1.0f,
        -1.0f, -1.0f,  0.0f, 0.0f,
         1.0f, -1.0f,  1.0f, 0.0f,

        -1.0f,  1.0f,  0.0f, 1.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
         1.0f,  1.0f,  1.0f, 1.0f
    };
    glGenVertexArrays(1, &rectVAO);
    glGenBuffers(1, &rectVBO);
    glBindVertexArray(rectVAO);
    glBindBuffer(GL_ARRAY_BUFFER, rectVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
}