#pragma once
#include <glm/glm.hpp>
#include <SFML/Window.hpp>
#include "Map.h" // Include Map to check for collisions

class Player {
public:
    // Constructor: Spawns player at specific location
    Player(glm::vec3 startPos);

    // Main Loop functions
    void HandleInput(const sf::Window& window);
    void Update(float dt, const Map& map);

    // Camera & Rendering Getters
    glm::mat4 GetViewMatrix() const;
    glm::vec3 GetPosition() const { return m_Position; }
    glm::vec3 GetFront() const { return m_Front; }

    // Flashlight Hand Position (Calculated for sway effect)
    glm::vec3 GetFlashlightPosition() const;

    // Gameplay Getters
    float GetBattery() const { return m_Battery; }
    bool IsDead() const { return m_Battery <= 0.0f; }
    void Recharge() { m_Battery = MAX_BATTERY; }
    void Reset(glm::vec3 startPos);

private:
    // --- Physics State ---
    glm::vec3 m_Position;
    glm::vec3 m_Velocity;

    // --- Camera Vectors ---
    glm::vec3 m_Front;
    glm::vec3 m_Up;
    glm::vec3 m_Right;
    glm::vec3 m_WorldUp;

    // --- View State ---
    float m_Yaw;
    float m_Pitch;

    // --- Gameplay State ---
    float m_Battery;
    bool m_IsGrounded;
    float m_HeadBobTimer;
    bool m_IsSprinting;

    // --- Configuration Constants ---
    static constexpr float WALK_SPEED = 2.0f;
    static constexpr float RUN_SPEED = 3.5f;
    static constexpr float MOUSE_SENSITIVITY = 0.1f;
    static constexpr float PLAYER_HEIGHT = 1.6f;
    static constexpr float PLAYER_RADIUS = 0.35f; // Fat enough to not peek through walls
    static constexpr float GRAVITY = 20.0f;
    static constexpr float JUMP_FORCE = 6.5f;
    static constexpr float MAX_BATTERY = 180.0f;

    // Internal Helpers
    void ProcessMouseLook(const sf::Window& window);
    void UpdateCameraVectors();
};