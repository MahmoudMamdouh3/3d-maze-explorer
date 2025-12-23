#include "Game.h"
#include "ResourceManager.h"
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

Game::Game()
    : m_State(GameState::MENU),
      m_PaperPos(8.5f, 0.5f, 8.5f),
      m_UIText(m_Font),
      m_CenterText(m_Font)
{
    // --- 1. Setup Window ---
    sf::ContextSettings settings;
    settings.depthBits = 24;
    settings.majorVersion = 3;
    settings.minorVersion = 3;
    settings.attributeFlags = sf::ContextSettings::Default;

    m_Window.create(sf::VideoMode({1280, 720}), "3D Maze - The Bureaucracy", sf::Style::Close, sf::State::Windowed, settings);
    m_Window.setFramerateLimit(144);
    m_Window.setMouseCursorVisible(true);

    // --- 2. Load OpenGL ---
    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(sf::Context::getFunction))) {
        throw std::runtime_error("Failed to initialize GLAD");
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // --- 3. Init Subsystems ---
    // Load Standard Shader (for Player/Paper)
    m_Shader = std::make_unique<Shader>();
    m_Shader->Load("assets/shaders/shader.vert", "assets/shaders/shader.frag");

    // Load Instanced Shader (for Walls) - AAA Optimization
    m_InstancedShader = std::make_unique<Shader>();
    m_InstancedShader->Load("assets/shaders/instanced.vert", "assets/shaders/shader.frag");

    m_Renderer = std::make_unique<Renderer>();
    m_Map = std::make_unique<Map>();
    m_Player = std::make_unique<Player>(glm::vec3(1.5f, 0.0f, 1.5f));

    // --- 4. Load Resources ---
    m_FloorTex = ResourceManager::LoadTexture("floor", "assets/textures/floor/fabricfloor.png");
    m_WallTex = ResourceManager::LoadTexture("wall", "assets/textures/wall/PaintedPlaster.png");
    m_CeilingTex = ResourceManager::LoadTexture("ceiling", "assets/textures/Ceiling/OfficeCeiling006_4K-PNG_Color.png");
    m_PaperTex = m_FloorTex;

    // --- 5. PRE-CALCULATE MAZE GEOMETRY (Optimization) ---
    // Instead of calculating this every frame, we do it once here.
    m_WallTransforms.clear();

    for (int x = 0; x < m_Map->GetWidth(); x++) {
        for (int z = 0; z < m_Map->GetHeight(); z++) {
            int tile = m_Map->GetTile(x, z);

            if (tile == 1) { // Wall
                glm::mat4 model = glm::mat4(1.0f);
                model = glm::translate(model, glm::vec3(x, 1.5f, z));
                model = glm::scale(model, glm::vec3(1.0f, 4.0f, 1.0f));
                m_WallTransforms.push_back(model);
            }
        }
    }

    // Send the massive list of matrices to the GPU once
    m_Renderer->SetupInstancedWalls(m_WallTransforms);

    // --- 6. Setup UI ---
    if (!m_Font.openFromFile("assets/textures/Font/font.TTF")) {
        std::cerr << "CRITICAL: Font not found at assets/textures/Font/font.TTF" << std::endl;
    }

    m_UIText.setCharacterSize(24);
    m_UIText.setFillColor(sf::Color::White);

    m_CenterText.setCharacterSize(40);
    m_CenterText.setFillColor(sf::Color::Red);
}

Game::~Game() {
    ResourceManager::Clear();
}

void Game::Run() {
    while (m_Window.isOpen()) {
        float dt = m_DeltaClock.restart().asSeconds();
        if (dt > 0.1f) dt = 0.1f;

        ProcessEvents();
        Update(dt);
        Render();
    }
}

void Game::ProcessEvents() {
    while (const std::optional event = m_Window.pollEvent()) {
        if (event->is<sf::Event::Closed>()) m_Window.close();

        if (const auto* keyEvent = event->getIf<sf::Event::KeyPressed>()) {
            if (keyEvent->scancode == sf::Keyboard::Scan::Escape) {
                if (m_State == GameState::PLAYING) {
                    m_State = GameState::PAUSED;
                    m_Window.setMouseCursorVisible(true);
                    m_Window.setMouseCursorGrabbed(false);
                } else if (m_State == GameState::PAUSED) {
                    m_State = GameState::PLAYING;
                    m_Window.setMouseCursorVisible(false);
                    m_Window.setMouseCursorGrabbed(true);
                }
            }

            if (m_State != GameState::PLAYING && m_State != GameState::PAUSED) {
                if (keyEvent->scancode == sf::Keyboard::Scan::Enter) {
                    ResetGame();
                }
            }
        }
    }
}

void Game::ResetGame() {
    m_State = GameState::PLAYING;
    m_Player->Reset(glm::vec3(1.5f, 0.0f, 1.5f));
    m_Window.setMouseCursorVisible(false);
    m_Window.setMouseCursorGrabbed(true);
}

void Game::Update(float dt) {
    if (m_State == GameState::PLAYING) {
        m_Player->HandleInput(m_Window);
        m_Player->Update(dt, *m_Map);

        if (glm::distance(m_Player->GetPosition(), m_PaperPos) < 1.0f) {
            m_State = GameState::WIN;
            m_Window.setMouseCursorVisible(true);
            m_Window.setMouseCursorGrabbed(false);
        }

        if (m_Player->IsDead()) {
            m_State = GameState::GAME_OVER;
            m_Window.setMouseCursorVisible(true);
            m_Window.setMouseCursorGrabbed(false);
        }
    }
}

void Game::Render() {
    glClearColor(0.005f, 0.005f, 0.01f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (m_State == GameState::PLAYING || m_State == GameState::PAUSED) {

        // --- 1. PREPARE COMMON UNIFORMS ---
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), 1280.0f / 720.0f, 0.01f, 100.0f);
        glm::mat4 view = m_Player->GetViewMatrix();

        // Flashlight & Battery Logic
        float batteryRatio = m_Player->GetBattery() / 180.0f;
        float flickerVal = (std::rand() % 100) / 100.0f > 0.9f ? 0.2f : 1.0f;

        // --- 2. INSTANCED RENDERING (The AAA Optimization) ---
        m_InstancedShader->Use();
        m_InstancedShader->SetMat4("projection", projection);
        m_InstancedShader->SetMat4("view", view);
        m_InstancedShader->SetVec3("viewPos", m_Player->GetPosition());

        // Lighting for Instanced Shader
        m_InstancedShader->SetVec3("spotLight.position", m_Player->GetFlashlightPosition());
        m_InstancedShader->SetVec3("spotLight.direction", m_Player->GetFront());
        m_InstancedShader->SetFloat("spotLight.cutOff", std::cos(glm::radians(12.5f)));
        m_InstancedShader->SetFloat("spotLight.outerCutOff", std::cos(glm::radians(25.0f)));
        m_InstancedShader->SetFloat("spotLight.constant", 1.0f);
        m_InstancedShader->SetFloat("spotLight.linear", 0.045f);
        m_InstancedShader->SetFloat("spotLight.quadratic", 0.0075f);
        m_InstancedShader->SetVec3("spotLight.ambient", glm::vec3(0.01f));
        m_InstancedShader->SetVec3("spotLight.diffuse", glm::vec3(1.0f, 0.95f, 0.8f));
        m_InstancedShader->SetVec3("spotLight.specular", glm::vec3(1.0f));
        m_InstancedShader->SetFloat("batteryRatio", batteryRatio);
        m_InstancedShader->SetFloat("flicker", flickerVal);

        // ONE DRAW CALL for all walls!
        m_Renderer->DrawInstancedWalls(*m_InstancedShader, m_WallTex, m_WallTransforms.size());


        // --- 3. STANDARD RENDERING (Floors/Ceilings/Objective) ---
        // (We stick to standard loop for floors for now until you instance them too)
        m_Shader->Use();
        m_Shader->SetMat4("projection", projection);
        m_Shader->SetMat4("view", view);
        m_Shader->SetVec3("viewPos", m_Player->GetPosition());
        // ... Set lighting for standard shader (same as above) ...
        m_Shader->SetVec3("spotLight.position", m_Player->GetFlashlightPosition());
        m_Shader->SetVec3("spotLight.direction", m_Player->GetFront());
        m_Shader->SetFloat("spotLight.cutOff", std::cos(glm::radians(12.5f)));
        m_Shader->SetFloat("spotLight.outerCutOff", std::cos(glm::radians(25.0f)));
        m_Shader->SetFloat("spotLight.constant", 1.0f);
        m_Shader->SetFloat("spotLight.linear", 0.045f);
        m_Shader->SetFloat("spotLight.quadratic", 0.0075f);
        m_Shader->SetVec3("spotLight.ambient", glm::vec3(0.01f));
        m_Shader->SetVec3("spotLight.diffuse", glm::vec3(1.0f, 0.95f, 0.8f));
        m_Shader->SetVec3("spotLight.specular", glm::vec3(1.0f));
        m_Shader->SetFloat("batteryRatio", batteryRatio);
        m_Shader->SetFloat("flicker", flickerVal);

        for (int x = 0; x < m_Map->GetWidth(); x++) {
            for (int z = 0; z < m_Map->GetHeight(); z++) {
                int tile = m_Map->GetTile(x, z);
                if (tile == 0) { // Path
                    glm::mat4 model = glm::mat4(1.0f);
                    model = glm::translate(model, glm::vec3(x, -0.5f, z));
                    m_Renderer->DrawCube(*m_Shader, model, m_FloorTex);

                    model = glm::mat4(1.0f);
                    model = glm::translate(model, glm::vec3(x, 3.5f, z));
                    model = glm::rotate(model, glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f));
                    m_Renderer->DrawCube(*m_Shader, model, m_CeilingTex);
                }
            }
        }

        // Render Objective
        glm::mat4 model = glm::mat4(1.0f);
        float floatY = m_PaperPos.y + std::sin(m_GameTime.getElapsedTime().asSeconds() * 2.0f) * 0.1f;
        model = glm::translate(model, glm::vec3(m_PaperPos.x, floatY, m_PaperPos.z));
        model = glm::scale(model, glm::vec3(0.3f, 0.01f, 0.4f));
        m_Renderer->DrawCube(*m_Shader, model, m_PaperTex);
    }

    glBindVertexArray(0);
    glUseProgram(0);

    RenderUI();
    m_Window.display();
}

void Game::RenderUI() {
    m_Window.pushGLStates();

    if (m_State == GameState::MENU) {
        m_CenterText.setString("THE BUREAUCRACY\n\nPress ENTER to Start");
        m_CenterText.setPosition({300.f, 200.f});
        m_Window.draw(m_CenterText);
    } else if (m_State == GameState::PLAYING) {
        m_UIText.setString("Battery: " + std::to_string(static_cast<int>(m_Player->GetBattery())) + "s");
        m_UIText.setPosition({10.f, 10.f});
        m_Window.draw(m_UIText);
    } else if (m_State == GameState::GAME_OVER) {
        m_CenterText.setString("LIGHTS OUT.\nPress ENTER to Retry");
        m_CenterText.setPosition({350.f, 300.f});
        m_Window.draw(m_CenterText);
    } else if (m_State == GameState::WIN) {
        m_CenterText.setString("FORM SUBMITTED.\nPress ENTER to Continue");
        m_CenterText.setPosition({250.f, 300.f});
        m_Window.draw(m_CenterText);
    }

    m_Window.popGLStates();
}