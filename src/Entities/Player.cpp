#include "Player.h"
#include "../Core/AudioManager.h" // FIX: Include full definition
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
    ProcessMouseLook(window);
    m_IsSprinting = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LShift);

    if (m_IsGrounded && sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Space)) {
        m_Velocity.y = JUMP_FORCE;
        m_IsGrounded = false;
    }
}

void Player::Update(float dt, const Map& map, AudioManager& audio) {
    float speed = m_IsSprinting ? RUN_SPEED : WALK_SPEED;
    glm::vec3 moveDir(0.0f);

    glm::vec3 flatFront = glm::normalize(glm::vec3(m_Front.x, 0.0f, m_Front.z));
    glm::vec3 flatRight = glm::normalize(glm::vec3(m_Right.x, 0.0f, m_Right.z));

    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W)) moveDir += flatFront;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S)) moveDir -= flatFront;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A)) moveDir -= flatRight;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D)) moveDir += flatRight;

    if (glm::length(moveDir) > 0.01f) {
        moveDir = glm::normalize(moveDir);
        float bobSpeed = speed * 5.0f;
        m_HeadBobTimer += dt * bobSpeed;

        // FOOTSTEPS
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

    glm::vec3 proposedPos = m_Position;
    proposedPos.x += moveDir.x * speed * dt;
    if (map.IsWall(proposedPos.x, m_Position.z, PLAYER_RADIUS)) proposedPos.x = m_Position.x;

    proposedPos.z += moveDir.z * speed * dt;
    if (map.IsWall(proposedPos.x, proposedPos.z, PLAYER_RADIUS)) proposedPos.z = m_Position.z;

    m_Position.x = proposedPos.x;
    m_Position.z = proposedPos.z;

    m_Velocity.y -= GRAVITY * dt;
    m_Position.y += m_Velocity.y * dt;

    if (m_Position.y < -0.5f) {
        m_Position.y = -0.5f;
        m_Velocity.y = 0.0f;
        m_IsGrounded = true;
    }

    m_Battery -= dt;
    if (m_Battery < 0.0f) m_Battery = 0.0f;
}

void Player::ProcessMouseLook(const sf::Window& window) {
    if (!window.hasFocus()) return;

    sf::Vector2i mousePos = sf::Mouse::getPosition(window);
    sf::Vector2i center(640, 360);

    float xOffset = static_cast<float>(mousePos.x - center.x) * MOUSE_SENSITIVITY;
    float yOffset = static_cast<float>(center.y - mousePos.y) * MOUSE_SENSITIVITY;

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
    glm::vec3 cameraPos = m_Position + glm::vec3(0.0f, PLAYER_HEIGHT + bobOffset, 0.0f);
    return glm::lookAt(cameraPos, cameraPos + m_Front, m_Up);
}

glm::vec3 Player::GetFlashlightPosition() const {
    float bobOffset = std::sin(m_HeadBobTimer) * 0.05f;
    glm::vec3 eyePos = m_Position + glm::vec3(0.0f, PLAYER_HEIGHT + bobOffset, 0.0f);
    return eyePos + (m_Right * 0.3f) + (m_Front * 0.2f) + glm::vec3(0.0f, -0.25f, 0.0f);
}