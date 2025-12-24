#include "Player.h"
#include "../Core/AudioManager.h"
#include "../Physics/AABB.h" // Critical for collision resolution
#include <cmath>
#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>

Player::Player(glm::vec3 startPos)
    : m_Position(startPos), m_Velocity(0.0f), m_TargetVelocity(0.0f),
      m_WorldUp(0.0f, 1.0f, 0.0f), m_Yaw(-90.0f), m_Pitch(0.0f),
      m_Battery(MAX_BATTERY), m_IsGrounded(false), m_HasRedKey(false),
      m_HeadBobTimer(0.0f), m_IsSprinting(false), m_FootstepTimer(0.0f),
      m_CurrentFOV(BASE_FOV) // <--- Init FOV
{
    UpdateCameraVectors();
}

void Player::Reset(glm::vec3 startPos) {
    m_Position = startPos;
    m_Velocity = glm::vec3(0.0f);
    m_Yaw = -90.0f;
    m_Pitch = 0.0f;
    m_Battery = MAX_BATTERY;
    m_IsGrounded = false;
    UpdateCameraVectors();
}

void Player::HandleInput(const sf::Window& window) {
    // 1. Mouse Look
    ProcessMouseLook(window);

    // 2. Sprint Check
    m_IsSprinting = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LShift);

    // 3. Jump (Only if grounded)
    if (m_IsGrounded && sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Space)) {
        m_Velocity.y = JUMP_FORCE;
        m_IsGrounded = false;
    }
}
// src/Entities/Player.cpp
void Player::Update(float dt, const Map& map, AudioManager& audio) {
    // ---------------------------------------------------------
    // 1. INPUT & VELOCITY (MOMENTUM)
    // ---------------------------------------------------------
    float targetSpeed = m_IsSprinting ? RUN_SPEED : WALK_SPEED;
    glm::vec3 inputDir(0.0f);

    glm::vec3 flatFront = glm::normalize(glm::vec3(m_Front.x, 0.0f, m_Front.z));
    glm::vec3 flatRight = glm::normalize(glm::vec3(m_Right.x, 0.0f, m_Right.z));

    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W)) inputDir += flatFront;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S)) inputDir -= flatFront;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A)) inputDir -= flatRight;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D)) inputDir += flatRight;

    if (glm::length(inputDir) > 0.01f) {
        inputDir = glm::normalize(inputDir);
        m_TargetVelocity.x = inputDir.x * targetSpeed;
        m_TargetVelocity.z = inputDir.z * targetSpeed;
    } else {
        m_TargetVelocity.x = 0.0f;
        m_TargetVelocity.z = 0.0f;
    }

    // Apply Momentum (Lerp)
    float smoothFactor = (glm::length(inputDir) > 0.01f) ? ACCELERATION : FRICTION;
    m_Velocity.x += (m_TargetVelocity.x - m_Velocity.x) * smoothFactor * dt;
    m_Velocity.z += (m_TargetVelocity.z - m_Velocity.z) * smoothFactor * dt;

    // ---------------------------------------------------------
    // 2. ROBUST PHYSICS SUB-STEPPING
    // ---------------------------------------------------------
    float timeRemaining = dt;
    const float TIME_STEP = 0.005f; // Run physics at 200Hz for precision

    while (timeRemaining > 0.0f) {
        float step = std::min(timeRemaining, TIME_STEP);

        // Gravity
        m_Velocity.y -= GRAVITY * step;

        // Player Bounds Size
        glm::vec3 size(PLAYER_RADIUS * 2.0f, PLAYER_HEIGHT, PLAYER_RADIUS * 2.0f);

        // --- SOLVE X AXIS ---
        m_Position.x += m_Velocity.x * step;
        AABB playerBoxX(m_Position, size);
        // Get walls with a slight buffer to catch fast movements
        auto walls = map.GetNearbyWalls(m_Position, 1.0f);

        for (const auto& wall : walls) {
            if (playerBoxX.Intersects(wall)) {
                float penetrationX = playerBoxX.GetPenetration(wall).x;
                // Push back with Epsilon (0.001f) to prevent sticking
                if (m_Velocity.x > 0) m_Position.x -= (penetrationX + 0.001f);
                else                  m_Position.x += (penetrationX + 0.001f);
                m_Velocity.x = 0.0f; // Stop momentum on impact
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
        if (m_Position.y < -0.5f) { // Floor Level
            m_Position.y = -0.5f;
            m_Velocity.y = 0.0f;
            m_IsGrounded = true;
        }

        timeRemaining -= step;
    }

    // ---------------------------------------------------------
    // 3. GAMEPLAY LOGIC (FOV, BOB, ETC)
    // ---------------------------------------------------------
    // Dynamic FOV
    float horizontalSpeed = glm::length(glm::vec2(m_Velocity.x, m_Velocity.z));
    float targetFOV = BASE_FOV + (horizontalSpeed / RUN_SPEED) * (SPRINT_FOV - BASE_FOV);
    m_CurrentFOV += (targetFOV - m_CurrentFOV) * 5.0f * dt;

    // Head Bob
    if (m_IsGrounded && horizontalSpeed > 0.1f) {
        m_HeadBobTimer += dt * horizontalSpeed * 2.5f;
        float stepInterval = m_IsSprinting ? 0.35f : 0.55f;
        m_FootstepTimer -= dt;
        if (m_FootstepTimer <= 0.0f) {
            audio.PlayGlobal("footstep", 30.0f + (horizontalSpeed * 5.0f));
            m_FootstepTimer = stepInterval;
        }
    } else {
        m_HeadBobTimer = 0.0f;
        m_FootstepTimer = 0.0f;
    }

    // Battery
    m_Battery -= dt;
    if (m_Battery < 0.0f) m_Battery = 0.0f;
}

void Player::ProcessMouseLook(const sf::Window& window) {
    if (!window.hasFocus()) return;

    sf::Vector2i mousePos = sf::Mouse::getPosition(window);
    sf::Vector2i center(640, 360); // Assuming 1280x720 window, center is half

    // Calculate offset
    float xOffset = static_cast<float>(mousePos.x - center.x) * MOUSE_SENSITIVITY;
    float yOffset = static_cast<float>(center.y - mousePos.y) * MOUSE_SENSITIVITY; // Reversed Y

    // Reset mouse to center to lock it
    sf::Mouse::setPosition(center, window);

    m_Yaw += xOffset;
    m_Pitch += yOffset;

    // Clamp Pitch (Prevent neck breaking)
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
    // Add Head Bob offset to the view matrix
    float bobOffset = std::sin(m_HeadBobTimer) * 0.05f;
    glm::vec3 cameraPos = m_Position + glm::vec3(0.0f, PLAYER_HEIGHT + bobOffset, 0.0f);

    return glm::lookAt(cameraPos, cameraPos + m_Front, m_Up);
}

glm::vec3 Player::GetFlashlightPosition() const {
    // Logic: Flashlight is held in the right hand, slightly forward and down
    float bobOffset = std::sin(m_HeadBobTimer) * 0.05f;
    glm::vec3 eyePos = m_Position + glm::vec3(0.0f, PLAYER_HEIGHT + bobOffset, 0.0f);

    // Offset: Right 0.3, Forward 0.2, Down 0.25
    return eyePos + (m_Right * 0.3f) + (m_Front * 0.2f) + glm::vec3(0.0f, -0.25f, 0.0f);
}