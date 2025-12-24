#pragma once
#include <SFML/Window.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "Map.h"
#include "../Core/AudioManager.h"

class Player {
public:
    Player(glm::vec3 startPos);

    void HandleInput(const sf::Window& window, float dt, AudioManager& audio);
    void Update(float dt, const Map& map, AudioManager& audio);
    void Reset(glm::vec3 startPos);

    // Getters
    glm::vec3 GetPosition() const { return m_Position; }
    glm::vec3 GetEyePosition() const;
    glm::vec3 GetFront() const { return m_Front; }
    glm::mat4 GetViewMatrix() const;
    glm::vec3 GetFlashlightPosition() const;

    float GetBattery() const { return m_Battery; }
    float GetStamina() const { return m_Stamina; }
    bool IsFlashlightOn() const { return m_IsFlashlightOn; }
    bool HasRedKey() const { return m_HasRedKey; }
    float GetCurrentFOV() const { return m_CurrentFOV; }
    bool IsDead() const { return m_Battery <= 0.0f; }

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
    bool m_IsFatigued; // NEW: Prevents running if stamina hit 0

    // Gameplay Attributes
    float m_Battery;
    float m_Stamina;
    bool  m_IsFlashlightOn;
    bool  m_HasRedKey;
    float m_FlashlightToggleTimer;

    // Animation
    float m_HeadBobTimer;
    float m_FootstepTimer;
    float m_BreathingTimer;

    // Constants (AAA Tuning)
    const float WALK_SPEED = 2.5f;
    const float RUN_SPEED = 3.8f;
    const float GRAVITY = 22.0f;      // NEW: Higher gravity = Snappier jump (Less floaty)
    const float JUMP_FORCE = 7.0f;    // Adjusted for new gravity
    const float PLAYER_HEIGHT = 1.9f; // NEW: Taller (+20cm)
    const float PLAYER_RADIUS = 0.3f;
    const float MAX_BATTERY = 180.0f;
    const float MAX_STAMINA = 100.0f;
    const float CEILING_HEIGHT = 4.0f; // Visual ceiling match
    const float BASE_FOV = 60.0f;      // NEW: Wider base FOV
};