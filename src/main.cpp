#include <glad/glad.h>
#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <cmath>

constexpr int MAZE_WIDTH = 10;
constexpr int MAZE_HEIGHT = 10;
constexpr float PLAYER_RADIUS = 0.35f;
constexpr float GRAVITY = 20.0f;
constexpr float JUMP_FORCE = 6.5f;
constexpr float PLAYER_EYE_HEIGHT = 1.6f;

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
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 lightDir = normalize(spotLight.position - FragPos);
    float theta = dot(lightDir, normalize(-spotLight.direction));
    float epsilon = spotLight.cutOff - spotLight.outerCutOff;
    float intensity = clamp((theta - spotLight.outerCutOff) / epsilon, 0.0, 1.0);
    float distance = length(spotLight.position - FragPos);
    float attenuation = 1.0 / (spotLight.constant + spotLight.linear * distance + spotLight.quadratic * (distance * distance));
    vec3 ambient = spotLight.ambient * texture(texture1, TexCoord).rgb;
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = spotLight.diffuse * diff * texture(texture1, TexCoord).rgb;
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
    vec3 specular = spotLight.specular * spec * texture(texture1, TexCoord).rgb;
    vec3 result = (ambient + (diffuse + specular) * intensity) * attenuation;
    float fogStart = 0.5;
    float fogEnd = 5.0;
    float fogFactor = clamp((distance - fogStart) / (fogEnd - fogStart), 0.0, 1.0);
    vec3 fogColor = vec3(0.01, 0.01, 0.02);
    vec3 finalColor = mix(result, fogColor, fogFactor);
    FragColor = vec4(finalColor, 1.0);
}
)glsl";

void checkShaderStatus(unsigned int shader, const std::string& type) {
    int success;
    char infoLog[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        std::cerr << "ERROR: Shader Compilation (" << type << "):\n" << infoLog << std::endl;
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
    auto width = static_cast<int>(image.getSize().x);
    auto height = static_cast<int>(image.getSize().y);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image.getPixelsPtr());
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    return textureID;
}

bool checkWallCollision(float x, float z, int maze[10][10]) {
    auto gridX = static_cast<int>(std::round(x));
    auto gridZ = static_cast<int>(std::round(z));
    if (gridX < 0 || gridX >= MAZE_WIDTH || gridZ < 0 || gridZ >= MAZE_HEIGHT) return true;
    if (maze[gridX][gridZ] == 1) {
        auto wallMinX = static_cast<float>(gridX) - 0.5f;
        auto wallMaxX = static_cast<float>(gridX) + 0.5f;
        auto wallMinZ = static_cast<float>(gridZ) - 0.5f;
        auto wallMaxZ = static_cast<float>(gridZ) + 0.5f;
        float playerMinX = x - PLAYER_RADIUS;
        float playerMaxX = x + PLAYER_RADIUS;
        float playerMinZ = z - PLAYER_RADIUS;
        float playerMaxZ = z + PLAYER_RADIUS;
        return playerMaxX > wallMinX && playerMinX < wallMaxX && playerMaxZ > wallMinZ && playerMinZ < wallMaxZ;
    }
    return false;
}

int main() {
    sf::ContextSettings settings;
    settings.depthBits = 24;
    settings.majorVersion = 3; settings.minorVersion = 3;
    settings.attributeFlags = sf::ContextSettings::Core;
    sf::RenderWindow window(sf::VideoMode({1280u, 720u}), "3D Maze - Kafkaesque", sf::Style::Close, sf::State::Windowed, settings);
    window.setFramerateLimit(144);
    window.setMouseCursorVisible(false);
    window.setMouseCursorGrabbed(true);
    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(sf::Context::getFunction))) return -1;
    glEnable(GL_DEPTH_TEST);
    unsigned int vShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vShader, 1, &vertexShaderSource, nullptr);
    glCompileShader(vShader);
    checkShaderStatus(vShader, "VERTEX");
    unsigned int fShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fShader, 1, &fragmentShaderSource, nullptr);
    glCompileShader(fShader);
    checkShaderStatus(fShader, "FRAGMENT");
    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vShader);
    glAttachShader(shaderProgram, fShader);
    glLinkProgram(shaderProgram);
    glUseProgram(shaderProgram);
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

    auto floorTexturePath = "assets/textures/floor/fabricfloor.png";
    auto wallTexturePath = "assets/textures/wall/PaintedPlaster.png";
    auto ceilingTexturePath  = "assets/textures/Ceiling/OfficeCeiling006_4K-PNG_Color.png";


    unsigned int floorTex = loadTexture(floorTexturePath);
    unsigned int wallTex = loadTexture(wallTexturePath);
    unsigned int ceilingTex = loadTexture(ceilingTexturePath);




    glm::vec3 playerPos = glm::vec3(1.5f, 0.0f, 1.5f);
    glm::vec3 playerVel = glm::vec3(0.0f);
    bool isGrounded = false;
    glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
    float yaw = -90.0f; float pitch = 0.0f;
    bool isWindowFocused = true;
    int maze[MAZE_HEIGHT][MAZE_WIDTH] = {
        {1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
        {1, 0, 0, 0, 1, 0, 0, 0, 0, 1},
        {1, 0, 1, 0, 1, 0, 1, 0, 0, 1},
        {1, 0, 1, 0, 0, 0, 1, 0, 1, 1},
        {1, 0, 1, 1, 1, 1, 1, 0, 1, 1},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {1, 1, 1, 0, 1, 1, 1, 0, 0, 1},
        {1, 0, 0, 0, 1, 0, 1, 0, 0, 1},
        {1, 0, 1, 1, 1, 0, 0, 0, 0, 1},
        {1, 1, 1, 1, 1, 1, 1, 1, 1, 1}
    };
    sf::Clock deltaClock;
    while (window.isOpen()) {
        float dt = deltaClock.restart().asSeconds();
        if (dt > 0.1f) dt = 0.1f;
        while (const std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) window.close();
            if (event->is<sf::Event::FocusLost>()) { isWindowFocused = false; window.setMouseCursorVisible(true); window.setMouseCursorGrabbed(false); }
            if (event->is<sf::Event::FocusGained>()) { isWindowFocused = true; window.setMouseCursorVisible(false); window.setMouseCursorGrabbed(true); }
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Escape)) window.close();
        if (isWindowFocused) {
            sf::Vector2i mousePos = sf::Mouse::getPosition(window);
            sf::Vector2i center(640, 360);
            auto xOffset = static_cast<float>(mousePos.x - center.x) * 0.1f;
            auto yOffset = static_cast<float>(center.y - mousePos.y) * 0.1f;
            yaw += xOffset; pitch += yOffset;
            if (pitch > 89.0f) pitch = 89.0f; if (pitch < -89.0f) pitch = -89.0f;
            sf::Mouse::setPosition(center, window);
            glm::vec3 front;
            front.x = std::cos(glm::radians(yaw)) * std::cos(glm::radians(pitch));
            front.y = std::sin(glm::radians(pitch));
            front.z = std::sin(glm::radians(yaw)) * std::cos(glm::radians(pitch));
            cameraFront = glm::normalize(front);
            float speed = 2.0f;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LShift)) speed = 3.5f;
            glm::vec3 forwardDir = glm::normalize(glm::vec3(cameraFront.x, 0.0f, cameraFront.z));
            glm::vec3 rightDir = glm::normalize(glm::cross(cameraFront, cameraUp));
            glm::vec3 moveDir = glm::vec3(0.0f);
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up)) moveDir += forwardDir;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down)) moveDir -= forwardDir;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left)) moveDir -= rightDir;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right)) moveDir += rightDir;
            if (glm::length(moveDir) > 0.1f) moveDir = glm::normalize(moveDir);
            if (isGrounded && sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Space)) { playerVel.y = JUMP_FORCE; isGrounded = false; }
            glm::vec3 proposedPos = playerPos;
            proposedPos.x += moveDir.x * speed * dt;
            if (checkWallCollision(proposedPos.x, proposedPos.z, maze)) proposedPos.x = playerPos.x;
            proposedPos.z += moveDir.z * speed * dt;
            if (checkWallCollision(proposedPos.x, proposedPos.z, maze)) proposedPos.z = playerPos.z;
            playerPos.x = proposedPos.x; playerPos.z = proposedPos.z;
            playerVel.y -= GRAVITY * dt; playerPos.y += playerVel.y * dt;
            float groundLevel = -0.5f;
            if (playerPos.y < groundLevel) { playerPos.y = groundLevel; playerVel.y = 0.0f; isGrounded = true; }
        }
        glClearColor(0.01f, 0.01f, 0.02f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glUseProgram(shaderProgram);
        glm::vec3 eyePos = playerPos + glm::vec3(0.0f, PLAYER_EYE_HEIGHT, 0.0f);
        glm::vec3 flatFront = glm::normalize(glm::vec3(cameraFront.x, 0.0f, cameraFront.z));
        glm::vec3 rightAxis = glm::normalize(glm::cross(flatFront, cameraUp));
        glm::vec3 lightOffset = (rightAxis * 0.3f) + (flatFront * 0.2f) + glm::vec3(0.0f, -0.2f, 0.0f);
        glm::vec3 flashLightPos = eyePos + lightOffset;
        glUniform3fv(glGetUniformLocation(shaderProgram, "viewPos"), 1, glm::value_ptr(eyePos));
        glUniform3fv(glGetUniformLocation(shaderProgram, "spotLight.position"), 1, glm::value_ptr(flashLightPos));
        glUniform3fv(glGetUniformLocation(shaderProgram, "spotLight.direction"), 1, glm::value_ptr(cameraFront));
        glUniform1f(glGetUniformLocation(shaderProgram, "spotLight.cutOff"), std::cos(glm::radians(10.0f)));
        glUniform1f(glGetUniformLocation(shaderProgram, "spotLight.outerCutOff"), std::cos(glm::radians(22.0f)));
        glUniform1f(glGetUniformLocation(shaderProgram, "spotLight.constant"), 1.0f);
        glUniform1f(glGetUniformLocation(shaderProgram, "spotLight.linear"), 0.045f);
        glUniform1f(glGetUniformLocation(shaderProgram, "spotLight.quadratic"), 0.0075f);
        glUniform3f(glGetUniformLocation(shaderProgram, "spotLight.ambient"), 0.02f, 0.02f, 0.03f);
        glUniform3f(glGetUniformLocation(shaderProgram, "spotLight.diffuse"), 1.0f, 0.98f, 0.9f);
        glUniform3f(glGetUniformLocation(shaderProgram, "spotLight.specular"), 1.0f, 1.0f, 1.0f);
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), 1280.0f/720.0f, 0.01f, 100.0f);
        glm::mat4 view = glm::lookAt(eyePos, eyePos + cameraFront, cameraUp);
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glBindVertexArray(VAO);
        for (int x = 0; x < MAZE_WIDTH; x++) {
            for (int z = 0; z < MAZE_HEIGHT; z++) {
                glm::mat4 model = glm::mat4(1.0f);
                if (maze[x][z] == 1) {
                    glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, wallTex); glUniform1i(glGetUniformLocation(shaderProgram, "texture1"), 0);
                    model = glm::translate(model, glm::vec3(x, 1.5f, z)); model = glm::scale(model, glm::vec3(1.0f, 4.0f, 1.0f));
                    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model)); glDrawArrays(GL_TRIANGLES, 0, 36);
                }
                glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, floorTex); glUniform1i(glGetUniformLocation(shaderProgram, "texture1"), 0);
                model = glm::mat4(1.0f); model = glm::translate(model, glm::vec3(x, -1.0f, z));
                glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model)); glDrawArrays(GL_TRIANGLES, 0, 36);
                glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, ceilingTex); glUniform1i(glGetUniformLocation(shaderProgram, "texture1"), 0);
                model = glm::mat4(1.0f); model = glm::translate(model, glm::vec3(x, 3.5f, z)); model = glm::rotate(model, glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f));
                glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model)); glDrawArrays(GL_TRIANGLES, 0, 36);
            }
        }
        window.display();
    }
    return 0;
}