#include "Game.h"
#include "ResourceManager.h"
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

Game::Game()
    : m_State(GameState::MENU),
      m_UIText(m_Font),
      m_CenterText(m_Font),
      m_InteractText(m_Font)
{
    // --- 1. Setup Window ---
    sf::ContextSettings settings;
    settings.depthBits = 24;
    settings.majorVersion = 3;
    settings.minorVersion = 3;
    settings.attributeFlags = sf::ContextSettings::Default;

    m_Window.create(sf::VideoMode({1280, 720}), "3D Maze - The Bureaucracy (AAA Edition)", sf::Style::Close, sf::State::Windowed, settings);
    m_Window.setFramerateLimit(144);
    m_Window.setMouseCursorVisible(true);

    std::random_device rd;
    m_RNG = std::mt19937(rd());

    // --- 2. Load OpenGL ---
    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(sf::Context::getFunction))) {
        throw std::runtime_error("Failed to initialize GLAD");
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // --- 3. Init Subsystems ---
    m_Shader = std::make_unique<Shader>();
    m_Shader->Load("assets/shaders/shader.vert", "assets/shaders/shader.frag");

    m_InstancedShader = std::make_unique<Shader>();
    m_InstancedShader->Load("assets/shaders/instanced.vert", "assets/shaders/shader.frag");

    m_Renderer = std::make_unique<Renderer>();
    m_Audio = std::make_unique<AudioManager>();
    m_PostProcessor = std::make_unique<PostProcessor>(1280, 720);

    m_Map = std::make_unique<Map>();

    // --- 4. Load Level ---
    if (!m_Map->LoadLevel("assets/levels/level1.txt", m_PlayerStartPos, m_PaperPos)) {
        throw std::runtime_error("FATAL: Failed to load assets/levels/level1.txt");
    }

    m_Player = std::make_unique<Player>(m_PlayerStartPos);

    // --- 5. Load Resources ---
    m_FloorTex = ResourceManager::LoadTexture("floor", "assets/textures/floor/fabricfloor.png");
    m_WallTex = ResourceManager::LoadTexture("wall", "assets/textures/wall/PaintedPlaster.png");
    m_CeilingTex = ResourceManager::LoadTexture("ceiling", "assets/textures/Ceiling/OfficeCeiling006_4K-PNG_Color.png");
    m_PaperTex = ResourceManager::LoadTexture("paper", "assets/textures/paper/paper.png");
    m_DoorTex = ResourceManager::LoadTexture("door", "assets/textures/door/Door001_8K-PNG_Color.png");

    // NEW TEXTURES
    m_LockedDoorTex = ResourceManager::LoadTexture("locked_door", "assets/textures/door/DoorLocked.png");
    m_KeyTex = ResourceManager::LoadTexture("key", "assets/textures/key/KeyCard.png");

    // --- 6. Load Audio Assets ---
    m_Audio->LoadSound("footstep", "assets/sounds/footstep.wav");
    m_Audio->LoadSound("hum", "assets/sounds/fluorescent_hum.wav");
    m_Audio->LoadSound("win", "assets/sounds/win.wav");
    m_Audio->LoadSound("lose", "assets/sounds/lose.wav");
    m_Audio->LoadSound("flicker", "assets/sounds/flicker.wav");

    // Play loopers
    m_Audio->PlayMusic("assets/sounds/ambience.ogg", 30.0f);
    m_Audio->PlaySpatial("hum", m_PaperPos, 100.0f, 2.0f);

    // --- 7. Pre-Calculate Instanced Geometry (Walls) ---
    m_WallTransforms.clear();
    for (int x = 0; x < m_Map->GetWidth(); x++) {
        for (int z = 0; z < m_Map->GetHeight(); z++) {
            if (m_Map->GetTile(x, z) == 1) {
                glm::mat4 model = glm::mat4(1.0f);
                // FIX: Add +0.5f to align mesh with the new physics center
                model = glm::translate(model, glm::vec3(x + 0.5f, 1.5f, z + 0.5f));
                model = glm::scale(model, glm::vec3(1.0f, 4.0f, 1.0f));
                m_WallTransforms.push_back(model);
            }
        }
    }
    m_Renderer->SetupInstancedWalls(m_WallTransforms);

    // --- 8. Setup UI ---
    if (!m_Font.openFromFile("assets/textures/Font/font.TTF")) {
        std::cerr << "CRITICAL: Font not found!" << std::endl;
    }
    m_UIText.setCharacterSize(24); m_UIText.setFillColor(sf::Color::White);
    m_CenterText.setCharacterSize(40); m_CenterText.setFillColor(sf::Color::Red);

    m_InteractText.setCharacterSize(30);
    m_InteractText.setFillColor(sf::Color::Yellow);

    // Fix: Use { } for vector parameters
    m_InteractText.setPosition({1280.0f / 2.0f - 100.0f, 720.0f / 2.0f + 50.0f});
    m_InteractText.setString("");
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

        if (const auto* resizeEvent = event->getIf<sf::Event::Resized>()) {
             glViewport(0, 0, static_cast<GLsizei>(resizeEvent->size.x), static_cast<GLsizei>(resizeEvent->size.y));
             m_PostProcessor->Resize(static_cast<int>(resizeEvent->size.x), static_cast<int>(resizeEvent->size.y));
        }

        if (const auto* keyEvent = event->getIf<sf::Event::KeyPressed>()) {
            if (keyEvent->scancode == sf::Keyboard::Scan::Escape) {
                if (m_State == GameState::PLAYING) {
                    m_State = GameState::PAUSED;
                    m_Window.setMouseCursorVisible(true); m_Window.setMouseCursorGrabbed(false);
                } else if (m_State == GameState::PAUSED) {
                    m_State = GameState::PLAYING;
                    m_Window.setMouseCursorVisible(false); m_Window.setMouseCursorGrabbed(true);
                }
            }
            if (m_State != GameState::PLAYING && m_State != GameState::PAUSED) {
                if (keyEvent->scancode == sf::Keyboard::Scan::Enter) ResetGame();
            }
        }
    }
}

void Game::ResetGame() {
    m_State = GameState::PLAYING;
    m_Player->Reset(m_PlayerStartPos);
    m_Window.setMouseCursorVisible(false); m_Window.setMouseCursorGrabbed(true);
}

void Game::Update(float dt) {
    m_Audio->UpdateListener(m_Player->GetPosition(), m_Player->GetFront(), glm::vec3(0,1,0));
    m_PostProcessor->Update(dt);

    if (m_State == GameState::PLAYING) {
        m_Player->HandleInput(m_Window);
        m_Player->Update(dt, *m_Map, *m_Audio);

        // --- GAMEPLAY MECHANICS ---

        // 1. Flashlight Flickering Audio
        if (m_Player->GetBattery() < 20.0f && m_Player->GetBattery() > 0.0f) {
            std::uniform_int_distribution<int> chance(0, 40);
            if (chance(m_RNG) == 0) {
                m_Audio->PlayGlobal("flicker", 60.0f);
            }
        }

        // 2. Key Pickup Logic
        int playerX = static_cast<int>(std::round(m_Player->GetPosition().x));
        int playerZ = static_cast<int>(std::round(m_Player->GetPosition().z));

        if (m_Map->GetTile(playerX, playerZ) == 4) { // Tile 4 is Key
            m_Player->PickUpRedKey();
            m_Map->SetTile(playerX, playerZ, 0); // Remove key from world
            m_Audio->PlayGlobal("win", 70.0f);
            m_UIText.setString("Acquired ACCESS KEY");
        }

        // 3. Interaction Raycast (Doors)
        m_InteractText.setString("");
        auto ray = m_Map->CastRay(m_Player->GetPosition(), m_Player->GetFront(), 2.5f);

        if (ray.hit) {
            if (ray.tileType == 2) { // Normal Door
                m_InteractText.setString("[E] Open Door");
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::E)) {
                    m_Map->SetTile(ray.tileX, ray.tileZ, 3); // Open
                    m_Audio->PlaySpatial("footstep", {ray.tileX, 1.5, ray.tileZ});
                }
            }
            else if (ray.tileType == 5) { // Locked Door
                if (m_Player->HasRedKey()) {
                    m_InteractText.setString("[E] UNLOCK Door");
                    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::E)) {
                        m_Map->SetTile(ray.tileX, ray.tileZ, 3); // Open
                        m_Audio->PlaySpatial("footstep", {ray.tileX, 1.5, ray.tileZ});
                    }
                } else {
                    m_InteractText.setString("LOCKED [Requires Red Key]");
                }
            }
        }

        // 4. Win/Lose Conditions
        if (glm::distance(m_Player->GetPosition(), m_PaperPos) < 1.0f) {
            m_Audio->PlayGlobal("win");
            m_State = GameState::WIN;
            m_Window.setMouseCursorVisible(true); m_Window.setMouseCursorGrabbed(false);
        }
        if (m_Player->IsDead()) {
            m_Audio->PlayGlobal("lose");
            m_State = GameState::GAME_OVER;
            m_Window.setMouseCursorVisible(true); m_Window.setMouseCursorGrabbed(false);
        }
    }
}

void Game::Render() {
    // 1. POST-PROCESSING CAPTURE
    bool usePostProcessing = (m_State == GameState::PLAYING || m_State == GameState::PAUSED);
    if (usePostProcessing) m_PostProcessor->BeginRender();
    else {
        glClearColor(0.005f, 0.005f, 0.01f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    if (m_State == GameState::PLAYING || m_State == GameState::PAUSED) {
        // USE DYNAMIC FOV HERE
        glm::mat4 projection = glm::perspective(glm::radians(m_Player->GetCurrentFOV()), 1280.0f / 720.0f, 0.01f, 100.0f);
        glm::mat4 view = m_Player->GetViewMatrix();
        float batteryRatio = m_Player->GetBattery() / 180.0f;

        std::uniform_real_distribution<float> dist(0.0f, 1.0f);
        float flickerVal = dist(m_RNG) > 0.9f ? 0.2f : 1.0f;

        // A. DRAW INSTANCED WALLS (These were already correct!)
        m_InstancedShader->Use();
        m_InstancedShader->SetMat4("projection", projection);
        m_InstancedShader->SetMat4("view", view);
        m_InstancedShader->SetVec3("viewPos", m_Player->GetPosition());

        // ... (Keep your Lighting Uniforms exactly as they were) ...
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

        m_Renderer->DrawInstancedWalls(*m_InstancedShader, m_WallTex, static_cast<GLsizei>(m_WallTransforms.size()));

        // B. DRAW DYNAMIC OBJECTS
        m_Shader->Use();
        m_Shader->SetMat4("projection", projection);
        m_Shader->SetMat4("view", view);
        m_Shader->SetVec3("viewPos", m_Player->GetPosition());

        // ... (Copy Lighting Uniforms to m_Shader here as well) ...
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

                // Floor & Ceiling
                if (tile == 0 || tile == 3 || tile == 4) {
                    glm::mat4 model = glm::mat4(1.0f);
                    // FIX 1: Add +0.5f to X and Z
                    model = glm::translate(model, glm::vec3(x + 0.5f, -0.5f, z + 0.5f));
                    m_Renderer->DrawCube(*m_Shader, model, m_FloorTex);

                    model = glm::mat4(1.0f);
                    // FIX 2: Add +0.5f to X and Z
                    model = glm::translate(model, glm::vec3(x + 0.5f, 3.5f, z + 0.5f));
                    model = glm::rotate(model, glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f));
                    m_Renderer->DrawCube(*m_Shader, model, m_CeilingTex);
                }

                // Doors (Normal & Locked)
                if (tile == 2 || tile == 5) {
                    unsigned int tex = (tile == 2) ? m_DoorTex : m_LockedDoorTex;

                    // 1. The Actual Door
                    glm::mat4 model = glm::mat4(1.0f);
                    // FIX 3: Add +0.5f to X and Z
                    model = glm::translate(model, glm::vec3(x + 0.5f, 0.75f, z + 0.5f));
                    model = glm::scale(model, glm::vec3(1.0f, 2.5f, 1.0f));
                    m_Renderer->DrawCube(*m_Shader, model, tex);

                    // 2. The Wall Filler Above
                    model = glm::mat4(1.0f);
                    // FIX 4: Add +0.5f to X and Z
                    model = glm::translate(model, glm::vec3(x + 0.5f, 2.75f, z + 0.5f));
                    model = glm::scale(model, glm::vec3(1.0f, 1.5f, 1.0f));
                    m_Renderer->DrawCube(*m_Shader, model, m_WallTex);
                }

                // Key (Tile 4)
                if (tile == 4) {
                    glm::mat4 model = glm::mat4(1.0f);
                    float floatY = -0.2f + std::sin(m_GameTime.getElapsedTime().asSeconds() * 3.0f) * 0.1f;
                    // FIX 5: Add +0.5f to X and Z
                    model = glm::translate(model, glm::vec3(x + 0.5f, floatY, z + 0.5f));
                    model = glm::scale(model, glm::vec3(0.2f, 0.2f, 0.2f));
                    m_Renderer->DrawCube(*m_Shader, model, m_KeyTex);
                }
            }
        }

        // Draw Objective Paper
        glm::mat4 model = glm::mat4(1.0f);
        float floatY = m_PaperPos.y + std::sin(m_GameTime.getElapsedTime().asSeconds() * 2.0f) * 0.1f;
        // PaperPos was already loaded with +0.5f in Map.cpp, so this is safe!
        model = glm::translate(model, glm::vec3(m_PaperPos.x, floatY, m_PaperPos.z));
        model = glm::scale(model, glm::vec3(0.3f, 0.01f, 0.4f));
        m_Renderer->DrawCube(*m_Shader, model, m_PaperTex);
    }

    glBindVertexArray(0);
    glUseProgram(0);

    // 3. APPLY POST-PROCESSING
    if (usePostProcessing) m_PostProcessor->EndRender();

    // 4. UI
    RenderUI();
    m_Window.display();
}

void Game::RenderUI() {
    m_Window.pushGLStates();

    // 1. MENU STATE
    if (m_State == GameState::MENU) {
        m_CenterText.setString("THE BUREAUCRACY\n\nPress ENTER to Start");
        // FIX: SFML 3.0 requires Vector2f for setPosition
        m_CenterText.setPosition({300.f, 200.f});
        m_Window.draw(m_CenterText);
    }
    // 2. PLAYING STATE
    else if (m_State == GameState::PLAYING) {

        // --- A. CROSSHAIR ---
        sf::CircleShape crosshair(3.0f); // 3px radius dot
        // FIX: SFML 3.0 requires Vector2f for setOrigin
        crosshair.setOrigin({3.0f, 3.0f});
        // FIX: SFML 3.0 requires Vector2f for setPosition
        crosshair.setPosition({1280.0f / 2.0f, 720.0f / 2.0f});

        if (!m_InteractText.getString().isEmpty()) {
            crosshair.setFillColor(sf::Color::Red);
            // FIX: SFML 3.0 requires Vector2f for setScale
            crosshair.setScale({1.5f, 1.5f});
            // FIX: SFML 3.0 requires Vector2f for setPosition
            m_InteractText.setPosition({1280.0f/2.0f + 20.0f, 720.0f/2.0f + 20.0f});
            m_Window.draw(m_InteractText);
        } else {
            crosshair.setFillColor(sf::Color(200, 200, 200, 150));
        }
        m_Window.draw(crosshair);

        // --- B. GRAPHICAL BATTERY BAR ---
        float barWidth = 200.0f;
        float barHeight = 20.0f;
        sf::Vector2f barPos(20.0f, 720.0f - barHeight - 40.0f);

        // 1. Background (Dark Grey)
        sf::RectangleShape backBar(sf::Vector2f(barWidth, barHeight));
        backBar.setPosition(barPos);
        backBar.setFillColor(sf::Color(50, 50, 50, 200));
        backBar.setOutlineColor(sf::Color::White);
        backBar.setOutlineThickness(2.0f);
        m_Window.draw(backBar);

        // 2. Foreground (Color changes based on %)
        float batteryPct = m_Player->GetBattery() / 180.0f;
        sf::RectangleShape frontBar(sf::Vector2f(barWidth * batteryPct, barHeight));
        frontBar.setPosition(barPos);

        if (batteryPct > 0.5f) frontBar.setFillColor(sf::Color::Green);
        else if (batteryPct > 0.2f) frontBar.setFillColor(sf::Color::Yellow);
        else frontBar.setFillColor(sf::Color::Red);

        m_Window.draw(frontBar);

        // 3. Label
        m_UIText.setString("FLASHLIGHT");
        // FIX: SFML 3.0 requires Vector2f for setPosition
        m_UIText.setPosition({barPos.x, barPos.y - 30.0f});
        // FIX: SFML 3.0 requires Vector2f for setScale
        m_UIText.setScale({0.8f, 0.8f});
        m_Window.draw(m_UIText);

        // --- C. KEYCARD ICON ---
        if (m_Player->HasRedKey()) {
             sf::RectangleShape keyIcon(sf::Vector2f(30.0f, 40.0f));
             // FIX: SFML 3.0 requires Vector2f for setPosition
             keyIcon.setPosition({barPos.x + barWidth + 20.0f, barPos.y - 10.0f});
             keyIcon.setFillColor(sf::Color::Red);
             keyIcon.setOutlineColor(sf::Color::White);
             keyIcon.setOutlineThickness(2.0f);
             m_Window.draw(keyIcon);

             m_UIText.setString("KEY");
             // FIX: SFML 3.0 requires Vector2f for setPosition
             m_UIText.setPosition({barPos.x + barWidth + 20.0f, barPos.y - 35.0f});
             m_Window.draw(m_UIText);
        }

    }
    // 3. GAME OVER / WIN
    else if (m_State == GameState::GAME_OVER) {
        m_CenterText.setString("LIGHTS OUT.\nPress ENTER to Retry");
        // FIX: SFML 3.0 requires Vector2f for setPosition
        m_CenterText.setPosition({350.f, 300.f});
        m_Window.draw(m_CenterText);
    } else if (m_State == GameState::WIN) {
        m_CenterText.setString("FORM SUBMITTED.\nPress ENTER to Continue");
        // FIX: SFML 3.0 requires Vector2f for setPosition
        m_CenterText.setPosition({250.f, 300.f});
        m_Window.draw(m_CenterText);
    }

    m_Window.popGLStates();
}