#include "Player.h"
#include "../Core/AudioManager.h"
#include "../Physics/AABB.h"
#include <cmath>
#include <algorithm>
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>

Player::Player(glm::vec3 startPos)
    : m_Position(startPos), m_Velocity(0.0f), m_TargetVelocity(0.0f),
      m_WorldUp(0.0f, 1.0f, 0.0f), m_Yaw(-90.0f), m_Pitch(0.0f),
      m_Battery(MAX_BATTERY), m_Stamina(MAX_STAMINA),
      m_IsGrounded(false), m_HasRedKey(false), m_IsFlashlightOn(true),
      m_HeadBobTimer(0.0f), m_IsSprinting(false), m_IsFatigued(false),
      m_FootstepTimer(0.0f),
      m_CurrentFOV(BASE_FOV), m_FlashlightToggleTimer(0.0f), m_BreathingTimer(0.0f)
{
    UpdateCameraVectors();
}

void Player::Reset(glm::vec3 startPos) {
    m_Position = startPos;
    m_Velocity = glm::vec3(0.0f);
    m_Yaw = -90.0f;
    m_Pitch = 0.0f;
    m_Battery = MAX_BATTERY;
    m_Stamina = MAX_STAMINA;
    m_IsFlashlightOn = true;
    m_IsGrounded = false;
    m_HasRedKey = false;
    m_IsFatigued = false;
    UpdateCameraVectors();
}

void Player::HandleInput(const sf::Window& window, float dt, AudioManager& audio) {
    ProcessMouseLook(window);

    if (m_FlashlightToggleTimer > 0.0f) m_FlashlightToggleTimer -= dt;

    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Scan::F) && m_FlashlightToggleTimer <= 0.0f) {
        m_IsFlashlightOn = !m_IsFlashlightOn;
        m_FlashlightToggleTimer = 0.3f;
        audio.PlayGlobal("click", 80.0f);
    }

    // --- FATIGUE SYSTEM ---
    bool shiftPressed = sf::Keyboard::isKeyPressed(sf::Keyboard::Scan::LShift);

    // If we hit 0 stamina, we are fatigued until we recover 30%
    if (m_Stamina <= 0.0f) m_IsFatigued = true;
    if (m_IsFatigued && m_Stamina > 30.0f) m_IsFatigued = false;

    // Can only sprint if NOT fatigued and Shift is held
    if (shiftPressed && !m_IsFatigued && m_Stamina > 0.0f) {
        m_IsSprinting = true;
    } else {
        m_IsSprinting = false;
    }

    // Jump
    if (m_IsGrounded && sf::Keyboard::isKeyPressed(sf::Keyboard::Scan::Space)) {
        m_Velocity.y = JUMP_FORCE;
        m_IsGrounded = false;
    }
}

void Player::Update(float dt, const Map& map, AudioManager& audio) {
    // 1. STAMINA (Fast Drain, Slow Recharge)
    if (m_IsSprinting && glm::length(glm::vec2(m_Velocity.x, m_Velocity.z)) > 0.1f) {
        m_Stamina -= dt * 35.0f; // Drains in ~3 seconds (Very Fast)
        if (m_Stamina < 0.0f) m_Stamina = 0.0f;
    } else {
        m_Stamina += dt * 7.0f; // Recharges in ~14 seconds (Slow)
        if (m_Stamina > MAX_STAMINA) m_Stamina = MAX_STAMINA;
    }

    // 2. MOVEMENT
    // CHANGED: Explicitly set speeds here to ensure they are faster/smoother
    // overriding the header macros for now.
    float runSpeed = 5.5f;  // Faster sprint
    float walkSpeed = 2.5f; // Standard walk
    float targetSpeed = m_IsSprinting ? runSpeed : walkSpeed;

    glm::vec3 inputDir(0.0f);
    glm::vec3 flatFront = glm::normalize(glm::vec3(m_Front.x, 0.0f, m_Front.z));
    glm::vec3 flatRight = glm::normalize(glm::vec3(m_Right.x, 0.0f, m_Right.z));

    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Scan::W)) inputDir += flatFront;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Scan::S)) inputDir -= flatFront;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Scan::A)) inputDir -= flatRight;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Scan::D)) inputDir += flatRight;

    if (glm::length(inputDir) > 0.01f) {
        inputDir = glm::normalize(inputDir);
        m_TargetVelocity.x = inputDir.x * targetSpeed;
        m_TargetVelocity.z = inputDir.z * targetSpeed;
    } else {
        m_TargetVelocity.x = 0.0f;
        m_TargetVelocity.z = 0.0f;
    }

    // Momentum
    if (glm::length(inputDir) < 0.01f && glm::length(glm::vec2(m_Velocity.x, m_Velocity.z)) < 0.1f) {
        m_Velocity.x = 0.0f;
        m_Velocity.z = 0.0f;
    } else {
        // Slightly tighter control (10.0f -> 12.0f)
        float smoothFactor = (glm::length(inputDir) > 0.01f) ? 12.0f : 10.0f;
        m_Velocity.x += (m_TargetVelocity.x - m_Velocity.x) * smoothFactor * dt;
        m_Velocity.z += (m_TargetVelocity.z - m_Velocity.z) * smoothFactor * dt;
    }

    // 3. PHYSICS
    float timeRemaining = dt;
    const float TIME_STEP = 0.005f;

    while (timeRemaining > 0.0f) {
        float step = std::min(timeRemaining, TIME_STEP);

        m_Velocity.y -= GRAVITY * step;

        // Player height used for collision box
        glm::vec3 size(PLAYER_RADIUS * 2.0f, PLAYER_HEIGHT, PLAYER_RADIUS * 2.0f);

        // X Axis
        m_Position.x += m_Velocity.x * step;
        AABB playerBoxX(m_Position, size);
        auto walls = map.GetNearbyWalls(m_Position, 1.0f);
        for (const auto& wall : walls) {
            if (playerBoxX.Intersects(wall)) {
                float penetrationX = playerBoxX.GetPenetration(wall).x;
                if (m_Velocity.x > 0) m_Position.x -= (penetrationX + 0.001f);
                else                  m_Position.x += (penetrationX + 0.001f);
                m_Velocity.x = 0.0f;
                break;
            }
        }

        // Z Axis
        m_Position.z += m_Velocity.z * step;
        AABB playerBoxZ(m_Position, size);
        for (const auto& wall : walls) {
            if (playerBoxZ.Intersects(wall)) {
                float penetrationZ = playerBoxZ.GetPenetration(wall).z;
                if (m_Velocity.z > 0) m_Position.z -= (penetrationZ + 0.001f);
                else                  m_Position.z += (penetrationZ + 0.001f);
                m_Velocity.z = 0.0f;
                break;
            }
        }

        // Y Axis
        m_Position.y += m_Velocity.y * step;
        if (m_Position.y < -0.5f) {
            m_Position.y = -0.5f;
            m_Velocity.y = 0.0f;
            m_IsGrounded = true;
        }

        // Ceiling Bonk (Using constant)
        float headTop = m_Position.y + PLAYER_HEIGHT;
        if (headTop > CEILING_HEIGHT) {
             m_Position.y = CEILING_HEIGHT - PLAYER_HEIGHT;
             m_Velocity.y = -0.5f;
        }

        timeRemaining -= step;
    }

    // 4. ANIMATION
    float horizontalSpeed = glm::length(glm::vec2(m_Velocity.x, m_Velocity.z));
    m_CurrentFOV += ((BASE_FOV + (horizontalSpeed / RUN_SPEED) * 12.0f) - m_CurrentFOV) * 5.0f * dt;

    if (m_IsGrounded && horizontalSpeed > 0.1f) {
        float bobSpeed = m_IsSprinting ? 16.0f : 10.0f;
        m_HeadBobTimer += dt * bobSpeed;

        float stepInterval = m_IsSprinting ? 0.40f : 0.65f;
        m_FootstepTimer -= dt;

        if (m_FootstepTimer <= 0.0f) {
            float vol = 30.0f + (horizontalSpeed * 5.0f);
            audio.PlayGlobal("footstep", vol);
            m_FootstepTimer = stepInterval;
        }
    } else {
        m_HeadBobTimer = 0.0f;
        m_FootstepTimer = 0.0f;
    }

    if (m_IsFatigued) {
        m_BreathingTimer -= dt;
        if (m_BreathingTimer <= 0.0f) {
             // Heavier breathing when fatigued
             // audio.PlayGlobal("breath", 40.0f);
             m_BreathingTimer = 1.5f;
        }
    }

    if (m_IsFlashlightOn) {
        m_Battery -= dt;
        if (m_Battery < 0.0f) m_Battery = 0.0f;
    }
}

void Player::ProcessMouseLook(const sf::Window& window) {
    if (!window.hasFocus()) return;

    sf::Vector2i mousePos = sf::Mouse::getPosition(window);
    sf::Vector2u size = window.getSize();
    sf::Vector2i center(size.x / 2, size.y / 2);

    // CHANGED: Sensitivity greatly reduced (0.07 -> 0.02)
    float sensitivity = 0.02f;
    float xOffset = static_cast<float>(mousePos.x - center.x) * sensitivity;
    float yOffset = static_cast<float>(center.y - mousePos.y) * sensitivity;

    sf::Mouse::setPosition(center, window);

    m_Yaw += xOffset;
    m_Pitch += yOffset;

    if (m_Pitch > 89.0f) m_Pitch = 89.0f;
    if (m_Pitch < -89.0f) m_Pitch = -89.0f;

    UpdateCameraVectors();
}

void Player::UpdateCameraVectors() {
    glm::vec3 front;
    front.x = cos(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));
    front.y = sin(glm::radians(m_Pitch));
    front.z = sin(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));
    m_Front = glm::normalize(front);

    m_Right = glm::normalize(glm::cross(m_Front, m_WorldUp));
    m_Up    = glm::normalize(glm::cross(m_Right, m_Front));
}

glm::mat4 Player::GetViewMatrix() const {
    float bobOffset = std::sin(m_HeadBobTimer) * 0.05f;
    // Eye is closer to top of head (Height 1.9 -> Eye 1.8)
    glm::vec3 eyePos = m_Position + glm::vec3(0.0f, 1.8f + bobOffset, 0.0f);
    return glm::lookAt(eyePos, eyePos + m_Front, m_Up);
}

glm::vec3 Player::GetEyePosition() const {
    float bobOffset = std::sin(m_HeadBobTimer) * 0.05f;
    return m_Position + glm::vec3(0.0f, 1.8f + bobOffset, 0.0f);
}

glm::vec3 Player::GetFlashlightPosition() const {
    float headBobY = std::sin(m_HeadBobTimer) * 0.08f;
    glm::vec3 eyePos = m_Position + glm::vec3(0.0f, 1.8f + headBobY, 0.0f);

    float handSwayX = std::cos(m_HeadBobTimer * 0.5f) * 0.15f;

    glm::vec3 handPos = eyePos
                      + (m_Right * (0.2f + handSwayX))
                      + (m_Front * 0.2f)
                      + (m_Up * (-0.3f));

    return handPos;
}