#pragma once
#include <SFML/Graphics.hpp>
#include <memory>
#include <vector>
#include "../Graphics/Shader.h"
#include "../Graphics/Renderer.h"
#include "../Entities/Player.h"
#include "../Entities/Map.h"
#include "../Core/AudioManager.h"
#include "../Graphics/PostProcessor.h"

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
    std::unique_ptr<Shader> m_Shader;          // Standard Shader (Objects)
    std::unique_ptr<Shader> m_InstancedShader; // Optimized Shader (Walls)
    std::unique_ptr<Renderer> m_Renderer;
    std::unique_ptr<AudioManager> m_Audio;
    std::unique_ptr<PostProcessor> m_PostProcessor;

    // Resources
    unsigned int m_FloorTex;
    unsigned int m_WallTex;
    unsigned int m_CeilingTex;
    unsigned int m_PaperTex;

    // Data for Rendering
    std::vector<glm::mat4> m_WallTransforms;

    // Gameplay Data
    glm::vec3 m_PaperPos;
    glm::vec3 m_PlayerStartPos;

    // UI Resources
    sf::Font m_Font;
    sf::Text m_UIText;
    sf::Text m_CenterText;
};