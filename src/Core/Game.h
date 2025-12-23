#pragma once
#include <SFML/Graphics.hpp>
#include <memory>
#include "../Graphics/Shader.h"
#include "../Graphics/Renderer.h"
#include "../Entities/Player.h"
#include "../Entities/Map.h"
#include "../Graphics/PostProcessor.h"
#include "../Core/AudioManager.h"


enum class GameState { MENU, PLAYING, PAUSED, GAME_OVER, WIN };

class Game {
public:
    Game();
    ~Game();

    void Run();

private:
    // Core Loop
    void ProcessEvents();
    void Update(float dt);
    void Render();
    void RenderUI();

    std::unique_ptr<Shader> m_InstancedShader; // NEW
    std::vector<glm::mat4> m_WallTransforms;   // NEW
    std::vector<glm::mat4> m_FloorTransforms;  // NEW
    std::unique_ptr<AudioManager> m_Audio;

    std::unique_ptr<PostProcessor> m_PostProcessor;


    // Helpers
    void ResetGame();

    // Window & State
    sf::RenderWindow m_Window;
    GameState m_State;
    sf::Clock m_DeltaClock;
    sf::Clock m_GameTime;

    // Subsystems
    std::unique_ptr<Player> m_Player;
    std::unique_ptr<Map> m_Map;
    std::unique_ptr<Shader> m_Shader;
    std::unique_ptr<Renderer> m_Renderer;

    // Resources
    unsigned int m_FloorTex;
    unsigned int m_WallTex;
    unsigned int m_CeilingTex;
    unsigned int m_PaperTex;

    // UI Resources (ORDER MATTERS FOR INITIALIZATION)
    sf::Font m_Font;        // Must be before Texts
    sf::Text m_UIText;
    sf::Text m_CenterText;

    // Objective
    glm::vec3 m_PaperPos;
};