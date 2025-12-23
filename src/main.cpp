#include <glad/glad.h>
#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <cmath>
#include <vector>
#include <string>

// --- GLOBAL CONSTANTS ---
constexpr int MAZE_WIDTH = 10;
constexpr int MAZE_HEIGHT = 10;
constexpr float PLAYER_RADIUS = 0.4f;
constexpr float GRAVITY = 22.0f;
constexpr float JUMP_FORCE = 7.0f;
constexpr float MAX_BATTERY = 180.0f;
constexpr float PLAYER_EYE_HEIGHT = 1.6f;

// --- GAME STATES ---
enum class GameState { MENU, PLAYING, PAUSED, GAME_OVER, WIN };

// --- SHADERS ---
const char* vertexShaderSource = R"glsl(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;
layout (location = 2) in vec3 aNormal;

out vec2 TexCoord;
out vec3 Normal;
out vec3 FragPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    FragPos = vec3(model * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(model))) * aNormal;
    TexCoord = aTexCoord;
    gl_Position = projection * view * vec4(FragPos, 1.0);
}
)glsl";

const char* fragmentShaderSource = R"glsl(
#version 330 core
out vec4 FragColor;

in vec2 TexCoord;
in vec3 Normal;
in vec3 FragPos;

uniform sampler2D texture1;
uniform vec3 viewPos;
uniform float batteryRatio;
uniform float flicker;

struct SpotLight {
    vec3 position;
    vec3 direction;
    float cutOff;
    float outerCutOff;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float constant;
    float linear;
    float quadratic;
};

uniform SpotLight spotLight;

void main() {
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(spotLight.position - FragPos);

    // Spotlight
    float theta = dot(lightDir, normalize(-spotLight.direction));
    float epsilon = spotLight.cutOff - spotLight.outerCutOff;
    float intensity = clamp((theta - spotLight.outerCutOff) / epsilon, 0.0, 1.0);

    float distance = length(spotLight.position - FragPos);
    float attenuation = 1.0 / (spotLight.constant + spotLight.linear * distance + spotLight.quadratic * (distance * distance));

    float powerFactor = clamp(batteryRatio, 0.0, 1.0);
    if (batteryRatio < 0.2) {
        powerFactor *= flicker;
    }

    vec3 ambient = spotLight.ambient * texture(texture1, TexCoord).rgb;

    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = spotLight.diffuse * diff * texture(texture1, TexCoord).rgb;

    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
    vec3 specular = spotLight.specular * spec * texture(texture1, TexCoord).rgb;

    vec3 result = ambient + (diffuse + specular) * intensity * attenuation * powerFactor;

    // Fog
    float fogStart = 0.5;
    float fogEnd = 7.0;
    float fogFactor = clamp((distance - fogStart) / (fogEnd - fogStart), 0.0, 1.0);
    vec3 fogColor = vec3(0.005, 0.005, 0.01);

    FragColor = vec4(mix(result, fogColor, fogFactor), 1.0);
}
)glsl";

// --- UTILITIES ---
void checkShaderStatus(unsigned int shader, const std::string& type) {
    int success;
    char infoLog[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        std::cerr << "ERROR: Shader (" << type << "): " << infoLog << std::endl;
    }
}

unsigned int loadTexture(const std::string& path) {
    sf::Image image;
    if (!image.loadFromFile(path)) {
        std::cerr << "ERROR: Texture not found: " << path << std::endl;
        return 0;
    }
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, static_cast<int>(image.getSize().x), static_cast<int>(image.getSize().y), 0, GL_RGBA, GL_UNSIGNED_BYTE, image.getPixelsPtr());
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    return textureID;
}

// --- PLAYER ---
struct Player {
    glm::vec3 position = glm::vec3(1.5f, 0.0f, 1.5f);
    glm::vec3 velocity = glm::vec3(0.0f);
    float yaw = -90.0f;
    float pitch = 0.0f;
    float battery = MAX_BATTERY;
    bool isGrounded = false;
    float headBobTimer = 0.0f;
};

// --- COLLISION ---
bool checkWallCollision(float x, float z, int maze[10][10]) {
    int gridX = static_cast<int>(std::round(x));
    int gridZ = static_cast<int>(std::round(z));
    if (gridX < 0 || gridX >= MAZE_WIDTH || gridZ < 0 || gridZ >= MAZE_HEIGHT) return true;
    if (maze[gridX][gridZ] == 1) {
        float wallMinX = static_cast<float>(gridX) - 0.5f;
        float wallMaxX = static_cast<float>(gridX) + 0.5f;
        float wallMinZ = static_cast<float>(gridZ) - 0.5f;
        float wallMaxZ = static_cast<float>(gridZ) + 0.5f;
        float pMinX = x - PLAYER_RADIUS; float pMaxX = x + PLAYER_RADIUS;
        float pMinZ = z - PLAYER_RADIUS; float pMaxZ = z + PLAYER_RADIUS;
        return (pMaxX > wallMinX && pMinX < wallMaxX && pMaxZ > wallMinZ && pMinZ < wallMaxZ);
    }
    return false;
}

int main() {
    // 1. SETUP
    sf::ContextSettings settings;
    settings.depthBits = 24; settings.majorVersion = 3; settings.minorVersion = 3;

    // *** FIX: Use Default (Compatibility) instead of Core to allow SFML Text ***
    settings.attributeFlags = sf::ContextSettings::Default;

    sf::RenderWindow window(sf::VideoMode({1280u, 720u}), "3D Maze - The Bureaucracy", sf::Style::Close, sf::State::Windowed, settings);
    window.setFramerateLimit(144);

    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(sf::Context::getFunction))) return -1;
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // 2. RESOURCES
    unsigned int vShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vShader, 1, &vertexShaderSource, nullptr); glCompileShader(vShader); checkShaderStatus(vShader, "VERTEX");
    unsigned int fShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fShader, 1, &fragmentShaderSource, nullptr); glCompileShader(fShader); checkShaderStatus(fShader, "FRAGMENT");
    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vShader); glAttachShader(shaderProgram, fShader); glLinkProgram(shaderProgram);

    // User Paths
    unsigned int floorTex = loadTexture("assets/textures/floor/fabricfloor.png");
    unsigned int wallTex = loadTexture("assets/textures/wall/PaintedPlaster.png");
    unsigned int ceilingTex = loadTexture("assets/textures/Ceiling/OfficeCeiling006_4K-PNG_Color.png");
    unsigned int paperTex = loadTexture("assets/textures/paper/paper.png");
    // Font Loading
    sf::Font font;
    if (!font.openFromFile("assets/textures/Font/font.TTF")) {
        std::cerr << "WARNING: font.TTF not found at assets/textures/Font/" << std::endl;
    }

    sf::Text uiText(font);
    uiText.setCharacterSize(24);
    uiText.setFillColor(sf::Color::White);

    sf::Text centerText(font);
    centerText.setCharacterSize(40);
    centerText.setFillColor(sf::Color::Red);

    float vertices[] = {
        -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,  0.0f,  0.0f, -1.0f,
         0.5f, -0.5f, -0.5f,  1.0f, 0.0f,  0.0f,  0.0f, -1.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 1.0f,  0.0f,  0.0f, -1.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 1.0f,  0.0f,  0.0f, -1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,  0.0f,  0.0f, -1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,  0.0f,  0.0f, -1.0f,

        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,  0.0f,  0.0f,  1.0f,
         0.5f, -0.5f,  0.5f,  1.0f, 0.0f,  0.0f,  0.0f,  1.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 1.0f,  0.0f,  0.0f,  1.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 1.0f,  0.0f,  0.0f,  1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f, 1.0f,  0.0f,  0.0f,  1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,  0.0f,  0.0f,  1.0f,

        -0.5f,  0.5f,  0.5f,  1.0f, 0.0f, -1.0f,  0.0f,  0.0f,
        -0.5f,  0.5f, -0.5f,  1.0f, 1.0f, -1.0f,  0.0f,  0.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f, -1.0f,  0.0f,  0.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f, -1.0f,  0.0f,  0.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f, -1.0f,  0.0f,  0.0f,
        -0.5f,  0.5f,  0.5f,  1.0f, 0.0f, -1.0f,  0.0f,  0.0f,

         0.5f,  0.5f,  0.5f,  1.0f, 0.0f,  1.0f,  0.0f,  0.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 1.0f,  1.0f,  0.0f,  0.0f,
         0.5f, -0.5f, -0.5f,  0.0f, 1.0f,  1.0f,  0.0f,  0.0f,
         0.5f, -0.5f, -0.5f,  0.0f, 1.0f,  1.0f,  0.0f,  0.0f,
         0.5f, -0.5f,  0.5f,  0.0f, 0.0f,  1.0f,  0.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 0.0f,  1.0f,  0.0f,  0.0f,

        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,  0.0f, -1.0f,  0.0f,
         0.5f, -0.5f, -0.5f,  1.0f, 1.0f,  0.0f, -1.0f,  0.0f,
         0.5f, -0.5f,  0.5f,  1.0f, 0.0f,  0.0f, -1.0f,  0.0f,
         0.5f, -0.5f,  0.5f,  1.0f, 0.0f,  0.0f, -1.0f,  0.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,  0.0f, -1.0f,  0.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,  0.0f, -1.0f,  0.0f,

        -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,  0.0f,  1.0f,  0.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 1.0f,  0.0f,  1.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 0.0f,  0.0f,  1.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 0.0f,  0.0f,  1.0f,  0.0f,
        -0.5f,  0.5f,  0.5f,  0.0f, 0.0f,  0.0f,  1.0f,  0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,  0.0f,  1.0f,  0.0f
    };

    unsigned int VBO, VAO;
    glGenVertexArrays(1, &VAO); glGenBuffers(1, &VBO);
    glBindVertexArray(VAO); glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), nullptr); glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), reinterpret_cast<void*>(3 * sizeof(float))); glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), reinterpret_cast<void*>(5 * sizeof(float))); glEnableVertexAttribArray(2);

    GameState state = GameState::MENU;
    Player player;
    sf::Clock deltaClock;

    int maze[MAZE_HEIGHT][MAZE_WIDTH] = {
        {1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
        {1, 0, 0, 0, 1, 0, 0, 0, 0, 1},
        {1, 0, 1, 0, 1, 0, 1, 1, 0, 1},
        {1, 0, 1, 0, 0, 0, 0, 0, 0, 1},
        {1, 0, 1, 1, 1, 1, 1, 1, 0, 1},
        {1, 0, 0, 0, 0, 0, 0, 1, 0, 1},
        {1, 1, 1, 1, 0, 1, 0, 1, 0, 1},
        {1, 0, 0, 0, 0, 1, 0, 0, 0, 1},
        {1, 0, 1, 1, 1, 1, 1, 1, 0, 1},
        {1, 1, 1, 1, 1, 1, 1, 1, 1, 1}
    };

    glm::vec3 paperPos = glm::vec3(8.5f, 0.5f, 8.5f);

    while (window.isOpen()) {
        float dt = deltaClock.restart().asSeconds();
        if (dt > 0.1f) dt = 0.1f;

        while (const std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) window.close();

            if (const auto* keyEvent = event->getIf<sf::Event::KeyPressed>()) {
                if (keyEvent->scancode == sf::Keyboard::Scan::Escape) {
                    if (state == GameState::PLAYING) {
                        state = GameState::PAUSED;
                        window.setMouseCursorVisible(true);
                        window.setMouseCursorGrabbed(false);
                    } else if (state == GameState::PAUSED) {
                        state = GameState::PLAYING;
                        window.setMouseCursorVisible(false);
                        window.setMouseCursorGrabbed(true);
                    }
                }

                if (state == GameState::MENU || state == GameState::GAME_OVER || state == GameState::WIN) {
                    if (keyEvent->scancode == sf::Keyboard::Scan::Enter) {
                        state = GameState::PLAYING;
                        player.position = glm::vec3(1.5f, 0.0f, 1.5f);
                        player.battery = MAX_BATTERY;
                        player.yaw = -90.0f;
                        player.pitch = 0.0f;
                        window.setMouseCursorVisible(false);
                        window.setMouseCursorGrabbed(true);
                    }
                }
            }
        }

        if (state == GameState::PLAYING) {
            sf::Vector2i mousePos = sf::Mouse::getPosition(window);
            sf::Vector2i center(640, 360);
            float xOffset = static_cast<float>(mousePos.x - center.x) * 0.1f;
            float yOffset = static_cast<float>(center.y - mousePos.y) * 0.1f;
            sf::Mouse::setPosition(center, window);

            player.yaw += xOffset;
            player.pitch += yOffset;
            if (player.pitch > 89.0f) player.pitch = 89.0f;
            if (player.pitch < -89.0f) player.pitch = -89.0f;

            glm::vec3 front;
            front.x = std::cos(glm::radians(player.yaw)) * std::cos(glm::radians(player.pitch));
            front.y = std::sin(glm::radians(player.pitch));
            front.z = std::sin(glm::radians(player.yaw)) * std::cos(glm::radians(player.pitch));
            glm::vec3 cameraFront = glm::normalize(front);
            glm::vec3 cameraRight = glm::normalize(glm::cross(cameraFront, glm::vec3(0.0f, 1.0f, 0.0f)));

            float speed = 2.0f;
            bool isMoving = false;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LShift)) speed = 3.5f;

            glm::vec3 moveDir = glm::vec3(0.0f);
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W)) { moveDir += glm::normalize(glm::vec3(cameraFront.x, 0.0f, cameraFront.z)); isMoving = true; }
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S)) { moveDir -= glm::normalize(glm::vec3(cameraFront.x, 0.0f, cameraFront.z)); isMoving = true; }
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A)) { moveDir -= cameraRight; isMoving = true; }
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D)) { moveDir += cameraRight; isMoving = true; }

            if (isMoving && player.isGrounded) {
                player.headBobTimer += dt * (speed * 5.0f);
            } else {
                player.headBobTimer = 0.0f;
            }

            if (glm::length(moveDir) > 0.01f) moveDir = glm::normalize(moveDir);

            glm::vec3 proposedPos = player.position;

            proposedPos.x += moveDir.x * speed * dt;
            if (checkWallCollision(proposedPos.x, proposedPos.z, maze)) proposedPos.x = player.position.x;

            proposedPos.z += moveDir.z * speed * dt;
            if (checkWallCollision(proposedPos.x, proposedPos.z, maze)) proposedPos.z = player.position.z;

            player.position.x = proposedPos.x;
            player.position.z = proposedPos.z;

            if (player.isGrounded && sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Space)) {
                player.velocity.y = JUMP_FORCE;
                player.isGrounded = false;
            }
            player.velocity.y -= GRAVITY * dt;
            player.position.y += player.velocity.y * dt;

            if (player.position.y < -0.5f) {
                player.position.y = -0.5f;
                player.velocity.y = 0.0f;
                player.isGrounded = true;
            }

            player.battery -= dt;
            if (player.battery <= 0.0f) state = GameState::GAME_OVER;

            if (glm::distance(player.position, paperPos) < 1.0f) {
                state = GameState::WIN;
            }
        }

        glClearColor(0.005f, 0.005f, 0.01f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if (state == GameState::PLAYING || state == GameState::PAUSED) {
            glUseProgram(shaderProgram);

            float bobOffset = std::sin(player.headBobTimer) * 0.05f;
            glm::vec3 eyePos = player.position + glm::vec3(0.0f, PLAYER_EYE_HEIGHT + bobOffset, 0.0f);

            glm::vec3 front;
            front.x = std::cos(glm::radians(player.yaw)) * std::cos(glm::radians(player.pitch));
            front.y = std::sin(glm::radians(player.pitch));
            front.z = std::sin(glm::radians(player.yaw)) * std::cos(glm::radians(player.pitch));
            glm::vec3 cameraFront = glm::normalize(front);
            glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

            glm::vec3 rightAxis = glm::normalize(glm::cross(cameraFront, cameraUp));
            glm::vec3 handPos = eyePos + (rightAxis * 0.3f) + (cameraFront * 0.2f) + glm::vec3(0.0f, -0.25f, 0.0f);

            glUniform3fv(glGetUniformLocation(shaderProgram, "viewPos"), 1, glm::value_ptr(eyePos));
            glUniform3fv(glGetUniformLocation(shaderProgram, "spotLight.position"), 1, glm::value_ptr(handPos));
            glUniform3fv(glGetUniformLocation(shaderProgram, "spotLight.direction"), 1, glm::value_ptr(cameraFront));

            glUniform1f(glGetUniformLocation(shaderProgram, "spotLight.cutOff"), std::cos(glm::radians(12.5f)));
            glUniform1f(glGetUniformLocation(shaderProgram, "spotLight.outerCutOff"), std::cos(glm::radians(25.0f)));
            glUniform1f(glGetUniformLocation(shaderProgram, "spotLight.constant"), 1.0f);
            glUniform1f(glGetUniformLocation(shaderProgram, "spotLight.linear"), 0.045f);
            glUniform1f(glGetUniformLocation(shaderProgram, "spotLight.quadratic"), 0.0075f);

            glUniform3f(glGetUniformLocation(shaderProgram, "spotLight.ambient"), 0.01f, 0.01f, 0.01f);
            glUniform3f(glGetUniformLocation(shaderProgram, "spotLight.diffuse"), 1.0f, 0.95f, 0.8f);
            glUniform3f(glGetUniformLocation(shaderProgram, "spotLight.specular"), 1.0f, 1.0f, 1.0f);

            float batteryRatio = player.battery / MAX_BATTERY;
            float flickerVal = (std::rand() % 100) / 100.0f > 0.9f ? 0.2f : 1.0f;
            glUniform1f(glGetUniformLocation(shaderProgram, "batteryRatio"), batteryRatio);
            glUniform1f(glGetUniformLocation(shaderProgram, "flicker"), flickerVal);

            glm::mat4 view = glm::lookAt(eyePos, eyePos + cameraFront, cameraUp);
            glm::mat4 projection = glm::perspective(glm::radians(45.0f), 1280.0f/720.0f, 0.01f, 100.0f);

            glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
            glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

            glBindVertexArray(VAO);
            for (int x = 0; x < MAZE_WIDTH; x++) {
                for (int z = 0; z < MAZE_HEIGHT; z++) {
                    glm::mat4 model = glm::mat4(1.0f);

                    if (maze[x][z] == 1) {
                        glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, wallTex); glUniform1i(glGetUniformLocation(shaderProgram, "texture1"), 0);
                        model = glm::translate(model, glm::vec3(x, 1.5f, z));
                        model = glm::scale(model, glm::vec3(1.0f, 4.0f, 1.0f));
                        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
                        glDrawArrays(GL_TRIANGLES, 0, 36);
                    } else {
                        glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, floorTex); glUniform1i(glGetUniformLocation(shaderProgram, "texture1"), 0);
                        model = glm::mat4(1.0f); model = glm::translate(model, glm::vec3(x, -0.5f, z));
                        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
                        glDrawArrays(GL_TRIANGLES, 0, 36);

                        glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, ceilingTex); glUniform1i(glGetUniformLocation(shaderProgram, "texture1"), 0);
                        model = glm::mat4(1.0f); model = glm::translate(model, glm::vec3(x, 3.5f, z));
                        model = glm::rotate(model, glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f));
                        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
                        glDrawArrays(GL_TRIANGLES, 0, 36);
                    }
                }
            }

            glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, paperTex); glUniform1i(glGetUniformLocation(shaderProgram, "texture1"), 0);
            glm::mat4 model = glm::mat4(1.0f);
            float floatY = paperPos.y + std::sin(deltaClock.getElapsedTime().asSeconds() * 2.0f) * 0.1f;
            model = glm::translate(model, glm::vec3(paperPos.x, floatY, paperPos.z));
            model = glm::scale(model, glm::vec3(0.3f, 0.01f, 0.4f));
            glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

        // *** FIX: Unbind VAO before letting SFML draw text to avoid state corruption ***
        glBindVertexArray(0);
        glUseProgram(0);

        window.pushGLStates();

        if (state == GameState::MENU) {
            centerText.setString("THE BUREAUCRACY\n\nPress ENTER to Start\nWASD to Move\nSHIFT to Run");
            centerText.setPosition({300.f, 200.f});
            window.draw(centerText);
        }
        else if (state == GameState::PLAYING) {
            uiText.setString("Battery: " + std::to_string(static_cast<int>(player.battery)) + "s");
            uiText.setPosition({10.f, 10.f});
            uiText.setFillColor(player.battery < 30.0f ? sf::Color::Red : sf::Color::Green);
            window.draw(uiText);

            uiText.setString("Find the Paper");
            uiText.setPosition({1000.f, 10.f});
            uiText.setFillColor(sf::Color::White);
            window.draw(uiText);
        }
        else if (state == GameState::PAUSED) {
            centerText.setString("PAUSED\nPress ESC to Continue");
            centerText.setPosition({400.f, 300.f});
            window.draw(centerText);
        }
        else if (state == GameState::GAME_OVER) {
            centerText.setString("LIGHTS OUT.\n\nYou were lost in the dark.\nPress ENTER to Retry");
            centerText.setPosition({350.f, 300.f});
            window.draw(centerText);
        }
        else {
            centerText.setString("FORM SUBMITTED.\n\nBut there is always more paperwork.\nPress ENTER to begin next shift.");
            centerText.setPosition({250.f, 300.f});
            window.draw(centerText);
        }

        window.popGLStates();
        window.display();
    }
    return 0;
}