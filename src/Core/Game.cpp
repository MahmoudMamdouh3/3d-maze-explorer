#include "Game.h"
#include "ResourceManager.h"
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

Game::Game()
    : m_State(GameState::MENU),
      m_UIText(m_Font),
      m_CenterText(m_Font),
      m_InteractText(m_Font),
      m_PauseMenuSelection(0),
      m_AudioStopped(false)
{
    sf::ContextSettings settings;
    settings.depthBits = 24;
    settings.majorVersion = 3;
    settings.minorVersion = 3;
    settings.antiAliasingLevel = 8; // CHANGED: 8x MSAA for smoother edges
    settings.attributeFlags = sf::ContextSettings::Default;

    sf::VideoMode desktop = sf::VideoMode::getDesktopMode();
    m_Window.create(desktop, "3D Maze - Mahmoud Mamdouh", sf::Style::Default, sf::State::Fullscreen, settings);
    m_Window.setFramerateLimit(165); // CHANGED: Match 165Hz monitor
    m_Window.setMouseCursorVisible(true);

    std::random_device rd;
    m_RNG = std::mt19937(rd());

    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(sf::Context::getFunction))) {
        throw std::runtime_error("Failed to initialize GLAD");
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    m_Shader = std::make_unique<Shader>();
    m_Shader->Load("assets/shaders/shader.vert", "assets/shaders/shader.frag");

    m_InstancedShader = std::make_unique<Shader>();
    m_InstancedShader->Load("assets/shaders/instanced.vert", "assets/shaders/shader.frag");

    m_Renderer = std::make_unique<Renderer>();
    m_Audio = std::make_unique<AudioManager>();

    m_PostProcessor = std::make_unique<PostProcessor>(desktop.size.x, desktop.size.y);

    m_Map = std::make_unique<Map>();

    if (!m_Map->LoadLevel("assets/levels/level1.txt", m_PlayerStartPos, m_PaperPos)) {
        throw std::runtime_error("FATAL: Failed to load assets/levels/level1.txt");
    }

    m_Player = std::make_unique<Player>(m_PlayerStartPos);

    m_FloorTex = ResourceManager::LoadTexture("floor", "assets/textures/floor/fabricfloor.png");
    m_WallTex = ResourceManager::LoadTexture("wall", "assets/textures/wall/PaintedPlaster.png");
    m_CeilingTex = ResourceManager::LoadTexture("ceiling", "assets/textures/Ceiling/OfficeCeiling006_4K-PNG_Color.png");
    m_PaperTex = ResourceManager::LoadTexture("paper", "assets/textures/paper/paper.png");
    m_DoorTex = ResourceManager::LoadTexture("door", "assets/textures/door/Door001_8K-PNG_Color.png");
    m_LockedDoorTex = ResourceManager::LoadTexture("locked_door", "assets/textures/door/DoorLocked.png");
    m_KeyTex = ResourceManager::LoadTexture("key", "assets/textures/key/KeyCard.png");

    m_Audio->LoadSound("footstep", "assets/sounds/footstep.wav");
    m_Audio->LoadSound("hum", "assets/sounds/fluorescent_hum.wav");
    m_Audio->LoadSound("win", "assets/sounds/win.wav");
    m_Audio->LoadSound("lose", "assets/sounds/lose.wav");
    m_Audio->LoadSound("flicker", "assets/sounds/flicker.wav");
    m_Audio->LoadSound("click", "assets/sounds/flashlight_click.wav");

    m_Audio->PlayMusic("assets/sounds/ambience.ogg", 25.0f);
    m_Audio->PlaySpatial("hum", m_PaperPos, 100.0f, 1.5f);

    m_WallTransforms.clear();
    for (int x = 0; x < m_Map->GetWidth(); x++) {
        for (int z = 0; z < m_Map->GetHeight(); z++) {
            if (m_Map->GetTile(x, z) == 1 || m_Map->GetTile(x, z) == 9) {
                glm::mat4 model = glm::mat4(1.0f);
                model = glm::translate(model, glm::vec3(x + 0.5f, 1.5f, z + 0.5f));
                model = glm::scale(model, glm::vec3(1.0f, 4.0f, 1.0f));
                m_WallTransforms.push_back(model);
            }
        }
    }
    m_Renderer->SetupInstancedWalls(m_WallTransforms);

    if (!m_Font.openFromFile("assets/textures/Font/font.TTF")) {
        std::cerr << "CRITICAL: Font not found!" << std::endl;
    }
    m_UIText.setCharacterSize(24); m_UIText.setFillColor(sf::Color::White);
    m_CenterText.setCharacterSize(40); m_CenterText.setFillColor(sf::Color::Red);
    m_InteractText.setCharacterSize(30); m_InteractText.setFillColor(sf::Color::Yellow);

    sf::Vector2u size = m_Window.getSize();
    m_InteractText.setPosition({static_cast<float>(size.x)/2.0f - 100.0f, static_cast<float>(size.y)/2.0f + 50.0f});
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
                    m_Audio->PlayGlobal("click", 50.0f);
                    m_Window.setMouseCursorVisible(true); m_Window.setMouseCursorGrabbed(false);
                } else if (m_State == GameState::PAUSED) {
                    m_State = GameState::PLAYING;
                    m_Audio->PlayGlobal("click", 50.0f);
                    m_Window.setMouseCursorVisible(false); m_Window.setMouseCursorGrabbed(true);
                }
            }

            if (m_State == GameState::PAUSED) {
                if (keyEvent->scancode == sf::Keyboard::Scan::W || keyEvent->scancode == sf::Keyboard::Scan::Up) {
                    m_PauseMenuSelection--;
                    if (m_PauseMenuSelection < 0) m_PauseMenuSelection = 2;
                    m_Audio->PlayGlobal("click", 50.0f);
                }
                if (keyEvent->scancode == sf::Keyboard::Scan::S || keyEvent->scancode == sf::Keyboard::Scan::Down) {
                    m_PauseMenuSelection++;
                    if (m_PauseMenuSelection > 2) m_PauseMenuSelection = 0;
                    m_Audio->PlayGlobal("click", 50.0f);
                }
                if (keyEvent->scancode == sf::Keyboard::Scan::Enter) {
                    m_Audio->PlayGlobal("click", 80.0f);
                    if (m_PauseMenuSelection == 0) {
                        m_State = GameState::PLAYING;
                        m_Window.setMouseCursorVisible(false); m_Window.setMouseCursorGrabbed(true);
                    }
                    else if (m_PauseMenuSelection == 1) {
                        ResetGame();
                    }
                    else if (m_PauseMenuSelection == 2) {
                        m_Window.close();
                    }
                }
            }

            if ((m_State == GameState::MENU || m_State == GameState::GAME_OVER || m_State == GameState::WIN) &&
                keyEvent->scancode == sf::Keyboard::Scan::Enter) {
                ResetGame();
            }
        }
    }
}

void Game::ResetGame() {
    m_State = GameState::PLAYING;
    m_Player->Reset(m_PlayerStartPos);
    m_Window.setMouseCursorVisible(false); m_Window.setMouseCursorGrabbed(true);

    m_Audio->StopAllSounds();
    m_AudioStopped = false;
    m_Audio->PlayMusic("assets/sounds/ambience.ogg", 25.0f);
    m_Audio->PlaySpatial("hum", m_PaperPos, 100.0f, 1.5f);
}

void Game::Update(float dt) {
    m_Audio->UpdateListener(m_Player->GetPosition(), m_Player->GetFront(), glm::vec3(0,1,0));
    m_PostProcessor->Update(dt);

    if (m_State == GameState::GAME_OVER || m_State == GameState::WIN) {
        if (!m_AudioStopped) {
            m_Audio->StopAllSounds();
            m_AudioStopped = true;
            if (m_State == GameState::WIN) m_Audio->PlayGlobal("win", 100.0f);
            if (m_State == GameState::GAME_OVER) m_Audio->PlayGlobal("lose", 100.0f);
        }
    } else {
        if (m_State == GameState::PLAYING) m_AudioStopped = false;
    }

    if (m_State == GameState::PLAYING) {
        m_Player->HandleInput(m_Window, dt, *m_Audio);
        m_Player->Update(dt, *m_Map, *m_Audio);

        if (m_Player->GetBattery() < 20.0f && m_Player->GetBattery() > 0.0f && m_Player->IsFlashlightOn()) {
            std::uniform_int_distribution<int> chance(0, 40);
            if (chance(m_RNG) == 0) {
                m_Audio->PlayGlobal("flicker", 60.0f);
            }
        }

        m_InteractText.setString("");
        auto ray = m_Map->CastRay(m_Player->GetEyePosition(), m_Player->GetFront(), 3.0f);

        if (ray.hit) {
            if (ray.tileType == 2) {
                m_InteractText.setString("[E] Open Door");
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::Scan::E)) {
                    m_Map->SetTile(ray.tileX, ray.tileZ, 3);
                    m_Audio->PlaySpatial("footstep", {ray.tileX, 1.5, ray.tileZ});
                }
            }
            else if (ray.tileType == 5) {
                if (m_Player->HasRedKey()) {
                    m_InteractText.setString("[E] UNLOCK Door");
                    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Scan::E)) {
                        m_Map->SetTile(ray.tileX, ray.tileZ, 3);
                        m_Audio->PlaySpatial("footstep", {ray.tileX, 1.5, ray.tileZ});
                    }
                } else {
                    m_InteractText.setString("LOCKED [Requires Access Key]");
                }
            }
        }

        int playerX = static_cast<int>(std::round(m_Player->GetPosition().x - 0.5f));
        int playerZ = static_cast<int>(std::round(m_Player->GetPosition().z - 0.5f));
        if (m_Map->GetTile(playerX, playerZ) == 4) {
            m_Player->PickUpRedKey();
            m_Map->SetTile(playerX, playerZ, 0);
            m_Audio->PlayGlobal("win", 70.0f);
            m_UIText.setString("Acquired ACCESS KEY");
        }

        if (glm::distance(m_Player->GetPosition(), m_PaperPos) < 1.0f) {
            m_State = GameState::WIN;
            m_Audio->StopAllSounds();
            m_Audio->PlayGlobal("win", 100.0f);
            m_Window.setMouseCursorVisible(true); m_Window.setMouseCursorGrabbed(false);
        }
        if (m_Player->IsDead()) {
            m_State = GameState::GAME_OVER;
            m_Audio->StopAllSounds();
            m_Audio->PlayGlobal("lose", 100.0f);
            m_Window.setMouseCursorVisible(true); m_Window.setMouseCursorGrabbed(false);
        }
    }
}

void Game::Render() {
    sf::Vector2u windowSize = m_Window.getSize();

    // 1. Handle Post-Processing State
    bool usePostProcessing = (m_State == GameState::PLAYING || m_State == GameState::PAUSED);

    if (usePostProcessing) {
        // This now binds the Multisampled FBO if you applied the Anti-Aliasing fix
        m_PostProcessor->BeginRender();
    } else {
        // Menu/Game Over screens draw directly to backbuffer
        glClearColor(0.005f, 0.005f, 0.01f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    // 2. Render 3D Scene
    if (m_State == GameState::PLAYING || m_State == GameState::PAUSED) {
        glm::mat4 projection = glm::perspective(glm::radians(m_Player->GetCurrentFOV()),
            static_cast<float>(windowSize.x) / static_cast<float>(windowSize.y), 0.01f, 100.0f);

        glm::mat4 view = m_Player->GetViewMatrix();

        float flashInt = (m_Player->IsFlashlightOn() && m_Player->GetBattery() > 0.0f) ? 1.0f : 0.0f;
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);
        if (m_Player->GetBattery() < 20.0f) flashInt *= (dist(m_RNG) > 0.9f ? 0.2f : 1.0f);

        // --- DRAW INSTANCED WALLS ---
        m_InstancedShader->Use();
        m_InstancedShader->SetMat4("projection", projection);
        m_InstancedShader->SetMat4("view", view);
        m_InstancedShader->SetVec3("viewPos", m_Player->GetPosition());

        // ADJUSTMENT: Brighter Ambient and Stronger Diffuse
        m_InstancedShader->SetVec3("spotLight.position", m_Player->GetFlashlightPosition());
        m_InstancedShader->SetVec3("spotLight.direction", m_Player->GetFront());
        m_InstancedShader->SetFloat("spotLight.cutOff", std::cos(glm::radians(12.5f)));
        m_InstancedShader->SetFloat("spotLight.outerCutOff", std::cos(glm::radians(25.0f)));
        m_InstancedShader->SetFloat("spotLight.constant", 1.0f);
        m_InstancedShader->SetFloat("spotLight.linear", 0.045f);
        m_InstancedShader->SetFloat("spotLight.quadratic", 0.0075f);
        // CHANGE THESE 3 LINES:
        m_InstancedShader->SetVec3("spotLight.ambient", glm::vec3(0.01f, 0.01f, 0.02f)); // Was 0.005f
        m_InstancedShader->SetVec3("spotLight.diffuse", glm::vec3(2.5f, 2.4f, 2.0f));    // Was 1.0f
        m_InstancedShader->SetVec3("spotLight.specular", glm::vec3(1.0f));
        m_InstancedShader->SetFloat("batteryRatio", flashInt);
        m_InstancedShader->SetFloat("flicker", 1.0f);

        m_Renderer->DrawInstancedWalls(*m_InstancedShader, m_WallTex, static_cast<GLsizei>(m_WallTransforms.size()));

        // --- DRAW SINGLE OBJECTS (Floor, Ceiling, Doors, Items) ---
        m_Shader->Use();
        m_Shader->SetMat4("projection", projection);
        m_Shader->SetMat4("view", view);
        m_Shader->SetVec3("viewPos", m_Player->GetPosition());

        m_Shader->SetVec3("spotLight.position", m_Player->GetFlashlightPosition());
        m_Shader->SetVec3("spotLight.direction", m_Player->GetFront());
        m_Shader->SetFloat("spotLight.cutOff", std::cos(glm::radians(12.5f)));
        m_Shader->SetFloat("spotLight.outerCutOff", std::cos(glm::radians(25.0f)));
        m_Shader->SetFloat("spotLight.constant", 1.0f);
        m_Shader->SetFloat("spotLight.linear", 0.045f);
        m_Shader->SetFloat("spotLight.quadratic", 0.0075f);
        m_Shader->SetVec3("spotLight.ambient", glm::vec3(0.01f, 0.01f, 0.02f)); // Was 0.005f
        m_Shader->SetVec3("spotLight.diffuse", glm::vec3(2.5f, 2.4f, 2.0f));    // Was 1.0f
        m_Shader->SetVec3("spotLight.specular", glm::vec3(1.0f));
        m_Shader->SetFloat("batteryRatio", flashInt);
        m_Shader->SetFloat("flicker", 1.0f);
        m_Shader->SetBool("isUnlit", false);

        for (int x = 0; x < m_Map->GetWidth(); x++) {
            for (int z = 0; z < m_Map->GetHeight(); z++) {
                int tile = m_Map->GetTile(x, z);

                if (tile == 0 || tile == 3 || tile == 4 || tile == 9) {
                    glm::mat4 model = glm::mat4(1.0f);
                    model = glm::translate(model, glm::vec3(x + 0.5f, -0.5f, z + 0.5f));
                    m_Renderer->DrawCube(*m_Shader, model, m_FloorTex);

                    model = glm::mat4(1.0f);
                    // FIXED GAP: Lowered Ceiling from 4.5f to 4.0f
                    model = glm::translate(model, glm::vec3(x + 0.5f, 4.0f, z + 0.5f));
                    model = glm::rotate(model, glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f));
                    m_Renderer->DrawCube(*m_Shader, model, m_CeilingTex);
                }

                if (tile == 2 || tile == 5) {
                    unsigned int tex = (tile == 2) ? m_DoorTex : m_LockedDoorTex;
                    glm::mat4 model = glm::mat4(1.0f);
                    model = glm::translate(model, glm::vec3(x + 0.5f, 0.75f, z + 0.5f));
                    model = glm::scale(model, glm::vec3(1.0f, 2.5f, 1.0f));
                    m_Renderer->DrawCube(*m_Shader, model, tex);

                    model = glm::mat4(1.0f);
                    model = glm::translate(model, glm::vec3(x + 0.5f, 2.75f, z + 0.5f));
                    model = glm::scale(model, glm::vec3(1.0f, 1.5f, 1.0f));
                    m_Renderer->DrawCube(*m_Shader, model, m_WallTex);
                }

                if (tile == 4) {
                    m_Shader->SetBool("isUnlit", true);
                    glm::mat4 model = glm::mat4(1.0f);
                    float floatY = 0.5f + std::sin(m_GameTime.getElapsedTime().asSeconds() * 2.0f) * 0.1f;
                    model = glm::translate(model, glm::vec3(x + 0.5f, floatY, z + 0.5f));
                    model = glm::rotate(model, m_GameTime.getElapsedTime().asSeconds(), glm::vec3(0,1,0));
                    model = glm::scale(model, glm::vec3(0.3f, 0.05f, 0.4f));
                    m_Renderer->DrawCube(*m_Shader, model, m_KeyTex);
                    m_Shader->SetBool("isUnlit", false);
                }
            }
        }

        glm::mat4 model = glm::mat4(1.0f);
        float floatY = m_PaperPos.y + std::sin(m_GameTime.getElapsedTime().asSeconds() * 2.0f) * 0.1f;
        model = glm::translate(model, glm::vec3(m_PaperPos.x, floatY, m_PaperPos.z));
        model = glm::scale(model, glm::vec3(0.3f, 0.01f, 0.4f));
        m_Renderer->DrawCube(*m_Shader, model, m_PaperTex);
    }

    glBindVertexArray(0);
    glUseProgram(0);

    if (usePostProcessing) {
        m_PostProcessor->EndRender();
    }

    RenderUI();
    m_Window.display();
}

void Game::RenderUI() {
    m_Window.pushGLStates();
    sf::Vector2u windowSize = m_Window.getSize();
    float centerX = windowSize.x / 2.0f;
    float centerY = windowSize.y / 2.0f;

    if (m_State == GameState::MENU) {
        m_UIText.setString("3d-maze-explorer \"By Mahmoud Mamdouh\"\n\n\nCONTROLS:\n\n\n[WASD] Move\n\n\n[F] Toggle Light\n\n\n[E] Open Locked Doors\n\n\nPRESS ENTER to Play");
        sf::FloatRect bounds = m_UIText.getLocalBounds();
        m_UIText.setPosition({centerX - bounds.size.x/2, centerY - bounds.size.y/2});
        m_Window.draw(m_UIText);

        if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Left)) {
            ResetGame();
        }
    }
    else if (m_State == GameState::PAUSED) {
        sf::RectangleShape overlay(sf::Vector2f(static_cast<float>(windowSize.x), static_cast<float>(windowSize.y)));
        overlay.setFillColor(sf::Color(0, 0, 0, 150));
        m_Window.draw(overlay);

        m_CenterText.setString("PAUSED");
        sf::FloatRect titleBounds = m_CenterText.getLocalBounds();
        m_CenterText.setPosition({centerX - titleBounds.size.x/2, centerY - 200});
        m_Window.draw(m_CenterText);

        std::vector<std::string> options = {"Resume", "Restart", "Quit"};
        for (int i = 0; i < 3; i++) {
            sf::Text opt = m_UIText;
            opt.setString(options[i]);
            if (i == m_PauseMenuSelection) opt.setFillColor(sf::Color::Yellow);
            else opt.setFillColor(sf::Color::White);

            sf::FloatRect b = opt.getLocalBounds();
            opt.setPosition({centerX - b.size.x/2, centerY + i * 50.0f});
            m_Window.draw(opt);
        }
    }
    else if (m_State == GameState::PLAYING) {
        sf::CircleShape crosshair(3.0f);
        crosshair.setOrigin({3.0f, 3.0f});
        crosshair.setPosition({centerX, centerY});
        if (!m_InteractText.getString().isEmpty()) {
            crosshair.setFillColor(sf::Color::Red);
            crosshair.setScale({1.5f, 1.5f});
            m_InteractText.setPosition({centerX + 20, centerY + 20});
            m_Window.draw(m_InteractText);
        } else {
            crosshair.setFillColor(sf::Color(200, 200, 200, 150));
        }
        m_Window.draw(crosshair);

        float barWidth = 200.0f;
        float barHeight = 20.0f;
        // ADJUSTMENT: Moved HUD higher (from -40.0f to -80.0f)
        sf::Vector2f barPos(20.0f, static_cast<float>(windowSize.y) - barHeight - 80.0f);

        sf::RectangleShape backBar(sf::Vector2f(barWidth, barHeight));
        backBar.setPosition(barPos);
        backBar.setFillColor(sf::Color(50, 50, 50, 200));
        backBar.setOutlineColor(sf::Color::White);
        backBar.setOutlineThickness(2.0f);
        m_Window.draw(backBar);

        float batteryPct = m_Player->GetBattery() / 180.0f;
        sf::RectangleShape frontBar(sf::Vector2f(barWidth * batteryPct, barHeight));
        frontBar.setPosition(barPos);
        if (batteryPct > 0.5f) frontBar.setFillColor(sf::Color::Green);
        else if (batteryPct > 0.2f) frontBar.setFillColor(sf::Color::Yellow);
        else frontBar.setFillColor(sf::Color::Red);
        m_Window.draw(frontBar);

        m_UIText.setString("FLASHLIGHT [F]");
        m_UIText.setPosition({barPos.x, barPos.y - 30.0f});
        m_UIText.setScale({0.8f, 0.8f});
        m_UIText.setFillColor(sf::Color::White);
        m_Window.draw(m_UIText);

        sf::Vector2f stamPos = barPos;
        stamPos.y += 35.0f; // Adjusted spacing

        // NEW: Add "STAMINA" label above bar
        m_UIText.setString("STAMINA");
        m_UIText.setPosition({stamPos.x, stamPos.y - 25.0f});
        m_UIText.setScale({0.6f, 0.6f});
        m_UIText.setFillColor(sf::Color::Cyan);
        m_Window.draw(m_UIText);

        sf::RectangleShape backStam(sf::Vector2f(barWidth, 10.0f));
        backStam.setPosition(stamPos);
        backStam.setFillColor(sf::Color(50, 50, 50, 200));
        m_Window.draw(backStam);

        float staminaPct = m_Player->GetStamina() / 100.0f;
        sf::RectangleShape frontStam(sf::Vector2f(barWidth * staminaPct, 10.0f));
        frontStam.setPosition(stamPos);
        frontStam.setFillColor(sf::Color::Cyan);
        m_Window.draw(frontStam);

        // Reset Text Settings for other draws
        m_UIText.setScale({1.0f, 1.0f});
        m_UIText.setFillColor(sf::Color::White);

        if (m_Player->HasRedKey()) {
             sf::RectangleShape keyIcon(sf::Vector2f(30.0f, 40.0f));
             keyIcon.setScale({1.5f, 1.5f});
             keyIcon.setPosition({barPos.x + barWidth + 20.0f, barPos.y - 20.0f});

             keyIcon.setFillColor(sf::Color(255, 215, 0));
             keyIcon.setOutlineColor(sf::Color::White);
             keyIcon.setOutlineThickness(2.0f);
             m_Window.draw(keyIcon);

             m_UIText.setString("ACCESS KEY");
             m_UIText.setPosition({barPos.x + barWidth + 80.0f, barPos.y - 15.0f});
             m_Window.draw(m_UIText);
        }
    }
    else if (m_State == GameState::GAME_OVER) {
        m_CenterText.setString("LIGHTS OUT.\n\n\nPress ENTER to Retry");
        sf::FloatRect bounds = m_CenterText.getLocalBounds();
        m_CenterText.setPosition({centerX - bounds.size.x/2, centerY - bounds.size.y/2});
        m_Window.draw(m_CenterText);
    }
    else if (m_State == GameState::WIN) {
        m_CenterText.setString("FORM SUBMITTED.\n\n\nPress ENTER to Continue");
        sf::FloatRect bounds = m_CenterText.getLocalBounds();
        m_CenterText.setPosition({centerX - bounds.size.x/2, centerY - bounds.size.y/2});
        m_Window.draw(m_CenterText);
    }

    m_Window.popGLStates();
}