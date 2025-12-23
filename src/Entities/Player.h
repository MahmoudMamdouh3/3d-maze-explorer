#pragma once
#include <glm/glm.hpp>
#include <SFML/Window.hpp>
#include "Map.h"

// Forward declaration
class AudioManager;

class Player {
public:
    Player(glm::vec3 startPos);

    void HandleInput(const sf::Window& window);

    // Updated signature to accept AudioManager
    void Update(float dt, const Map& map, AudioManager& audio);

    // Camera & Rendering Getters
    glm::mat4 GetViewMatrix() const; // This was missing in your error log
    glm::vec3 GetPosition() const { return m_Position; }
    glm::vec3 GetFront() const { return m_Front; }

    glm::vec3 GetFlashlightPosition() const;

    // Gameplay Getters
    float GetBattery() const { return m_Battery; }
    bool IsDead() const { return m_Battery <= 0.0f; }
    void Recharge() { m_Battery = MAX_BATTERY; }
    void Reset(glm::vec3 startPos);

private:
    glm::vec3 m_Position;
    glm::vec3 m_Velocity;
    glm::vec3 m_Front;
    glm::vec3 m_Up;
    glm::vec3 m_Right;
    glm::vec3 m_WorldUp;

    float m_Yaw;
    float m_Pitch;
    float m_Battery;
    bool m_IsGrounded;
    float m_HeadBobTimer;
    bool m_IsSprinting;
    float m_FootstepTimer;

    static constexpr float WALK_SPEED = 2.0f;
    static constexpr float RUN_SPEED = 3.5f;
    static constexpr float MOUSE_SENSITIVITY = 0.1f;
    static constexpr float PLAYER_HEIGHT = 1.6f;
    static constexpr float PLAYER_RADIUS = 0.35f;
    static constexpr float GRAVITY = 20.0f;
    static constexpr float JUMP_FORCE = 6.5f;
    static constexpr float MAX_BATTERY = 180.0f;

    void ProcessMouseLook(const sf::Window& window);
    void UpdateCameraVectors();
};