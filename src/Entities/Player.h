#pragma once
#include <SFML/Window.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "Map.h"
#include "../Core/AudioManager.h"

class Player {
public:
    Player(glm::vec3 startPos);

    void HandleInput(const sf::Window& window, float dt); // Added dt here for toggle timers
    void Update(float dt, const Map& map, AudioManager& audio);
    void Reset(glm::vec3 startPos);

    // Getters
    glm::vec3 GetPosition() const { return m_Position; }
    glm::vec3 GetFront() const { return m_Front; }
    glm::mat4 GetViewMatrix() const;
    glm::vec3 GetFlashlightPosition() const;

    // New Getters
    float GetBattery() const { return m_Battery; }
    float GetStamina() const { return m_Stamina; } // New
    bool IsFlashlightOn() const { return m_IsFlashlightOn; } // New
    bool HasRedKey() const { return m_HasRedKey; }
    float GetCurrentFOV() const { return m_CurrentFOV; }
    bool IsDead() const { return m_Battery <= 0.0f; }

    // Setters
    void PickUpRedKey() { m_HasRedKey = true; }

private:
    void ProcessMouseLook(const sf::Window& window);
    void UpdateCameraVectors();

    // Camera
    glm::vec3 m_Position;
    glm::vec3 m_Front, m_Up, m_Right, m_WorldUp;
    float m_Yaw, m_Pitch;
    float m_CurrentFOV;

    // Physics
    glm::vec3 m_Velocity;
    glm::vec3 m_TargetVelocity;
    bool m_IsGrounded;
    bool m_IsSprinting;

    // Gameplay Attributes
    float m_Battery;
    float m_Stamina;          // New: 0.0 to 100.0
    bool  m_IsFlashlightOn;   // New: Toggle state
    bool  m_HasRedKey;
    float m_FlashlightToggleTimer; // Prevent spamming 'F'

    // Animation / Feel
    float m_HeadBobTimer;
    float m_FootstepTimer;
    float m_BreathingTimer;   // New: For breathing sfx

    // Constants
    const float WALK_SPEED = 2.5f;
    const float RUN_SPEED = 5.0f;
    const float GRAVITY = 9.8f;
    const float PLAYER_HEIGHT = 1.7f;
    const float PLAYER_RADIUS = 0.3f;
    const float MAX_BATTERY = 180.0f;
    const float MAX_STAMINA = 100.0f;
};