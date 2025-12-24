#pragma once
#include <glm/glm.hpp>
#include <SFML/Window.hpp>
#include "Map.h"

// Forward declaration
class AudioManager;

class Player {
public:
    bool HasRedKey() const { return m_HasRedKey; }
    void PickUpRedKey() { m_HasRedKey = true; }
    float GetCurrentFOV() const { return m_CurrentFOV; }

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
    glm::vec3 m_Velocity; // Actual physical velocity
    glm::vec3 m_TargetVelocity; // Where input wants us to go

    glm::vec3 m_Front;
    glm::vec3 m_Up;
    glm::vec3 m_Right;
    glm::vec3 m_WorldUp;

    float m_Yaw;
    float m_Pitch;
    float m_Battery;
    bool m_IsGrounded;
    bool m_IsSprinting;
    bool m_HasRedKey;

    // Animation & Feel
    float m_HeadBobTimer;
    float m_FootstepTimer;
    float m_CurrentFOV; // NEW: For dynamic zoom

    // CONSTANTS - TWEAK THESE FOR FEEL
    static constexpr float WALK_SPEED = 2.5f;
    static constexpr float RUN_SPEED = 5.0f;     // Faster sprint
    static constexpr float ACCELERATION = 8.0f;  // How fast you get up to speed
    static constexpr float FRICTION = 6.0f;      // How fast you stop
    static constexpr float MOUSE_SENSITIVITY = 0.1f;
    static constexpr float PLAYER_HEIGHT = 1.6f;
    static constexpr float PLAYER_RADIUS = 0.3f; // Slightly smaller to fit doors better
    static constexpr float GRAVITY = 25.0f;      // Snappier gravity
    static constexpr float JUMP_FORCE = 8.0f;
    static constexpr float MAX_BATTERY = 180.0f;
    static constexpr float BASE_FOV = 45.0f;     // Standard zoom
    static constexpr float SPRINT_FOV = 55.0f;   // Zoom out when running

    void ProcessMouseLook(const sf::Window& window);
    void UpdateCameraVectors();
};