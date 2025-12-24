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
      m_Battery(MAX_BATTERY), m_Stamina(MAX_STAMINA), // Init Stamina
      m_IsGrounded(false), m_HasRedKey(false), m_IsFlashlightOn(true),
      m_HeadBobTimer(0.0f), m_IsSprinting(false), m_FootstepTimer(0.0f),
      m_CurrentFOV(45.0f), m_FlashlightToggleTimer(0.0f), m_BreathingTimer(0.0f)
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
    UpdateCameraVectors();
}

void Player::HandleInput(const sf::Window& window, float dt) {
    // 1. Mouse Look
    ProcessMouseLook(window);

    // 2. Flashlight Toggle (F)
    if (m_FlashlightToggleTimer > 0.0f) m_FlashlightToggleTimer -= dt;

    // SFML 3.0: Use Scan::F instead of Key::F
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Scan::F) && m_FlashlightToggleTimer <= 0.0f) {
        m_IsFlashlightOn = !m_IsFlashlightOn;
        m_FlashlightToggleTimer = 0.3f; // 300ms Cooldown prevents spam
    }

    // 3. Sprint Check (With Stamina)
    // SFML 3.0: Use Scan::LShift
    bool shiftPressed = sf::Keyboard::isKeyPressed(sf::Keyboard::Scan::LShift);

    // You can only sprint if you have stamina
    if (shiftPressed && m_Stamina > 0.0f) {
        m_IsSprinting = true;
    } else {
        m_IsSprinting = false;
    }

    // 4. Jump (Only if grounded)
    // SFML 3.0: Use Scan::Space
    if (m_IsGrounded && sf::Keyboard::isKeyPressed(sf::Keyboard::Scan::Space)) {
        m_Velocity.y = 5.0f; // JUMP_FORCE
        m_IsGrounded = false;
    }
}

void Player::Update(float dt, const Map& map, AudioManager& audio) {
    // ---------------------------------------------------------
    // 1. STAMINA LOGIC (Tweaked for Balance)
    // ---------------------------------------------------------
    // If moving AND sprinting, drain stamina
    if (m_IsSprinting && glm::length(glm::vec2(m_Velocity.x, m_Velocity.z)) > 0.1f) {
        m_Stamina -= dt * 25.0f; // Drains fast (approx 4 seconds)
        if (m_Stamina < 0.0f) m_Stamina = 0.0f;
    } else {
        // SLOWER RECHARGE (Requested: "a bit slower")
        m_Stamina += dt * 5.0f; // Takes 20 seconds to fully recharge from 0
        if (m_Stamina > MAX_STAMINA) m_Stamina = MAX_STAMINA;
    }

    // ---------------------------------------------------------
    // 2. MOVEMENT & MOMENTUM
    // ---------------------------------------------------------
    float targetSpeed = m_IsSprinting ? RUN_SPEED : WALK_SPEED;
    glm::vec3 inputDir(0.0f);

    glm::vec3 flatFront = glm::normalize(glm::vec3(m_Front.x, 0.0f, m_Front.z));
    glm::vec3 flatRight = glm::normalize(glm::vec3(m_Right.x, 0.0f, m_Right.z));

    // SFML 3.0: Use Scan codes
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

    // --- SMOOTH STOPPING FIX ---
    // If input is zero and velocity is low, SNAP to zero to prevent micro-sliding/jitter.
    if (glm::length(inputDir) < 0.01f && glm::length(glm::vec2(m_Velocity.x, m_Velocity.z)) < 0.1f) {
        m_Velocity.x = 0.0f;
        m_Velocity.z = 0.0f;
    } else {
        // Apply Momentum (Lerp)
        float smoothFactor = (glm::length(inputDir) > 0.01f) ? 10.0f : 10.0f; // Snappier movement
        m_Velocity.x += (m_TargetVelocity.x - m_Velocity.x) * smoothFactor * dt;
        m_Velocity.z += (m_TargetVelocity.z - m_Velocity.z) * smoothFactor * dt;
    }

    // ---------------------------------------------------------
    // 3. ROBUST PHYSICS SUB-STEPPING (AAA Collision)
    // ---------------------------------------------------------
    float timeRemaining = dt;
    const float TIME_STEP = 0.005f; // 200Hz Physics

    while (timeRemaining > 0.0f) {
        float step = std::min(timeRemaining, TIME_STEP);

        // Gravity
        m_Velocity.y -= GRAVITY * step;

        glm::vec3 size(PLAYER_RADIUS * 2.0f, PLAYER_HEIGHT, PLAYER_RADIUS * 2.0f);

        // --- SOLVE X AXIS ---
        m_Position.x += m_Velocity.x * step;
        AABB playerBoxX(m_Position, size);
        auto walls = map.GetNearbyWalls(m_Position, 1.0f);

        for (const auto& wall : walls) {
            if (playerBoxX.Intersects(wall)) {
                float penetrationX = playerBoxX.GetPenetration(wall).x;
                // Push back + Epsilon
                if (m_Velocity.x > 0) m_Position.x -= (penetrationX + 0.001f);
                else                  m_Position.x += (penetrationX + 0.001f);
                m_Velocity.x = 0.0f;
                break;
            }
        }

        // --- SOLVE Z AXIS ---
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

        // --- SOLVE Y AXIS (Floor) ---
        m_Position.y += m_Velocity.y * step;
        if (m_Position.y < -0.5f) {
            m_Position.y = -0.5f;
            m_Velocity.y = 0.0f;
            m_IsGrounded = true;
        }

        timeRemaining -= step;
    }

    // ---------------------------------------------------------
    // 4. ANIMATION & FEEL
    // ---------------------------------------------------------
    float horizontalSpeed = glm::length(glm::vec2(m_Velocity.x, m_Velocity.z));

    // Dynamic FOV
    float baseFov = 45.0f;
    float targetFOV = baseFov + (horizontalSpeed / RUN_SPEED) * 10.0f;
    m_CurrentFOV += (targetFOV - m_CurrentFOV) * 5.0f * dt;

    // Head Bob & Footsteps
    if (m_IsGrounded && horizontalSpeed > 0.1f) {
        // Sprinting = Faster Bob
        float bobSpeed = m_IsSprinting ? 16.0f : 10.0f;
        m_HeadBobTimer += dt * bobSpeed;

        float stepInterval = m_IsSprinting ? 0.45f : 0.65f; // Slower step interval
        m_FootstepTimer -= dt;

        if (m_FootstepTimer <= 0.0f) {
            // Volume increases with speed
            float vol = 30.0f + (horizontalSpeed * 5.0f);
            audio.PlayGlobal("footstep", vol);
            m_FootstepTimer = stepInterval;
        }
    } else {
        // Smoothly dampen head bob when stopping
        m_HeadBobTimer = 0.0f;
        m_FootstepTimer = 0.0f;
    }

    // Breathing Sound (Heavy breathing when tired)
    if (m_Stamina < 30.0f) {
        m_BreathingTimer -= dt;
        if (m_BreathingTimer <= 0.0f) {
             // Placeholder for breath sound
             // audio.PlayGlobal("breath", 20.0f);
             m_BreathingTimer = 2.5f;
        }
    }

    // Battery Logic
    if (m_IsFlashlightOn) {
        m_Battery -= dt;
        if (m_Battery < 0.0f) m_Battery = 0.0f;
    }
}

void Player::ProcessMouseLook(const sf::Window& window) {
    if (!window.hasFocus()) return;

    sf::Vector2i mousePos = sf::Mouse::getPosition(window);
    sf::Vector2i center(1280 / 2, 720 / 2); // Center of screen

    float xOffset = static_cast<float>(mousePos.x - center.x) * 0.1f; // Sensitivity
    float yOffset = static_cast<float>(center.y - mousePos.y) * 0.1f;

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
    // Add Head Bob offset to Camera Y
    float bobOffset = std::sin(m_HeadBobTimer) * 0.05f;
    // Base height 1.7
    glm::vec3 cameraPos = m_Position + glm::vec3(0.0f, 1.7f + bobOffset, 0.0f);

    return glm::lookAt(cameraPos, cameraPos + m_Front, m_Up);
}

// NEW: AAA Heavy Flashlight Sway
glm::vec3 Player::GetFlashlightPosition() const {
    // 1. Calculate Eye Position
    // Heavy Up/Down Bob
    float headBobY = std::sin(m_HeadBobTimer) * 0.08f;
    glm::vec3 eyePos = m_Position + glm::vec3(0.0f, 1.7f + headBobY, 0.0f);

    // 2. Hand Sway Logic (Heavy Drag)
    // Wide Left/Right sway based on cosine (circular motion relative to bob)
    float handSwayX = std::cos(m_HeadBobTimer * 0.5f) * 0.15f;

    // 3. Construct Hand Position
    // Right: 0.35 units + wide sway
    // Forward: 0.2 units
    // Down: -0.3 units (Lower down feels heavier)
    glm::vec3 handPos = eyePos
                      + (m_Right * (0.35f + handSwayX))
                      + (m_Front * 0.2f)
                      + (m_Up * (-0.3f)); // No Y-bob on hand relative to eye to simulate damping

    return handPos;
}