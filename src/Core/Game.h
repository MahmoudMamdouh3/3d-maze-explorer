#pragma once
#include <glad/glad.h>
#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <memory>
#include <random>

#include "../Graphics/Shader.h"
#include "../Graphics/Renderer.h"
#include "../Entities/Player.h"
#include "../Entities/Map.h"
#include "AudioManager.h"
#include "../Graphics/PostProcessor.h"

enum class GameState {
    MENU,
    PLAYING,
    PAUSED,
    GAME_OVER,
    WIN
};

class Game {
public:
    Game();
    ~Game();

    void Run();

private:
    void ProcessEvents();
    void Update(float dt);
    void Render();
    void RenderUI();
    void ResetGame();

    sf::RenderWindow m_Window;
    sf::Clock m_DeltaClock;
    sf::Clock m_GameTime;
    GameState m_State;
    std::mt19937 m_RNG;

    std::unique_ptr<Shader> m_Shader;
    std::unique_ptr<Shader> m_InstancedShader;
    std::unique_ptr<Renderer> m_Renderer;
    std::unique_ptr<AudioManager> m_Audio;
    std::unique_ptr<PostProcessor> m_PostProcessor;

    std::unique_ptr<Map> m_Map;
    std::unique_ptr<Player> m_Player;

    unsigned int m_FloorTex, m_WallTex, m_CeilingTex;
    unsigned int m_PaperTex, m_DoorTex, m_LockedDoorTex, m_KeyTex;
    std::vector<glm::mat4> m_WallTransforms;

    glm::vec3 m_PlayerStartPos;
    glm::vec3 m_PaperPos;

    sf::Font m_Font;
    sf::Text m_UIText;
    sf::Text m_CenterText;
    sf::Text m_InteractText;

    int m_PauseMenuSelection;
    bool m_AudioStopped;
};