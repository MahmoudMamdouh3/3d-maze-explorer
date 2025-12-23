#include "Player.h"
#include "../Core/AudioManager.h"
#include "../Physics/AABB.h" // Critical for collision resolution
#include <cmath>
#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>

Player::Player(glm::vec3 startPos)
    : m_Position(startPos), m_Velocity(0.0f), m_WorldUp(0.0f, 1.0f, 0.0f),
      m_Yaw(-90.0f), m_Pitch(0.0f), m_Battery(MAX_BATTERY),
      m_IsGrounded(false), m_HeadBobTimer(0.0f), m_IsSprinting(false),
      m_FootstepTimer(0.0f)
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
void Player::Update(float dt, const Map& map, AudioManager& audio) {
    // --- 1. Movement Input & Audio ---
    float speed = m_IsSprinting ? RUN_SPEED : WALK_SPEED;
    glm::vec3 inputDir(0.0f);

    glm::vec3 flatFront = glm::normalize(glm::vec3(m_Front.x, 0.0f, m_Front.z));
    glm::vec3 flatRight = glm::normalize(glm::vec3(m_Right.x, 0.0f, m_Right.z));

    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W)) inputDir += flatFront;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S)) inputDir -= flatFront;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A)) inputDir -= flatRight;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D)) inputDir += flatRight;

    if (glm::length(inputDir) > 0.01f) {
        inputDir = glm::normalize(inputDir);
        float bobSpeed = speed * 5.0f;
        m_HeadBobTimer += dt * bobSpeed;

        if (m_IsGrounded) {
             float stepInterval = m_IsSprinting ? 0.3f : 0.5f;
             m_FootstepTimer -= dt;
             if (m_FootstepTimer <= 0.0f) {
                 audio.PlayGlobal("footstep", 40.0f);
                 m_FootstepTimer = stepInterval;
             }
        }
    } else {
        m_HeadBobTimer = 0.0f;
        m_FootstepTimer = 0.0f;
    }

    // --- 2. Physics Setup ---
    // Prevent "Tunneling" by capping the physics step.
    // If lag spikes (dt > 0.05), we process physics in smaller safe chunks.
    float timeRemaining = dt;
    const float MAX_STEP = 0.02f; // Run physics at ~50Hz precision minimum

    while (timeRemaining > 0.0f) {
        float step = std::min(timeRemaining, MAX_STEP);

        // Apply Gravity
        m_Velocity.y -= GRAVITY * step;

        // Calculate Movement for this small step
        glm::vec3 stepMove = inputDir * speed * step;

        // --- PHASE A: MOVE X AXIS ---
        m_Position.x += stepMove.x;

        // Check Wall Collisions (X Only)
        // We create a box at the NEW X but OLD Z (Separated Axis)
        AABB playerBoxX(m_Position, glm::vec3(PLAYER_RADIUS * 2.0f, PLAYER_HEIGHT, PLAYER_RADIUS * 2.0f));
        auto wallsX = map.GetNearbyWalls(m_Position, 2.0f);

        for (const auto& originalWall : wallsX) {
            // Ignore Y axis by stretching wall vertically
            AABB wall = originalWall;
            wall.min.y = playerBoxX.min.y - 5.0f;
            wall.max.y = playerBoxX.max.y + 5.0f;

            if (playerBoxX.Intersects(wall)) {
                glm::vec3 penetration = playerBoxX.GetPenetration(wall);
                // We just moved X, so the collision MUST be on X.
                // Push back on X only.
                if (m_Position.x < wall.min.x + 0.5f) m_Position.x -= penetration.x;
                else m_Position.x += penetration.x;
            }
        }

        // --- PHASE B: MOVE Z AXIS ---
        m_Position.z += stepMove.z;

        // Check Wall Collisions (Z Only)
        // Now we are at Valid X, New Z.
        AABB playerBoxZ(m_Position, glm::vec3(PLAYER_RADIUS * 2.0f, PLAYER_HEIGHT, PLAYER_RADIUS * 2.0f));
        auto wallsZ = map.GetNearbyWalls(m_Position, 2.0f);

        for (const auto& originalWall : wallsZ) {
            AABB wall = originalWall;
            wall.min.y = playerBoxZ.min.y - 5.0f;
            wall.max.y = playerBoxZ.max.y + 5.0f;

            if (playerBoxZ.Intersects(wall)) {
                glm::vec3 penetration = playerBoxZ.GetPenetration(wall);
                // We just moved Z, so the collision MUST be on Z.
                // Push back on Z only.
                if (m_Position.z < wall.min.z + 0.5f) m_Position.z -= penetration.z;
                else m_Position.z += penetration.z;
            }
        }

        // --- PHASE C: MOVE Y AXIS ---
        m_Position.y += m_Velocity.y * step;

        // Floor Collision
        if (m_Position.y < -0.5f) {
            m_Position.y = -0.5f;
            m_Velocity.y = 0.0f;
            m_IsGrounded = true;
        }

        timeRemaining -= step;
    }

    // --- 3. Gameplay: Battery ---
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