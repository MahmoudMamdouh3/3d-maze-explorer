#pragma once
#include <glad/glad.h>
#include <string>
#include <unordered_map>
#include <glm/glm.hpp>

class Shader {
public:
    Shader();
    // Loads shaders from file paths (Production Grade)
    void Load(const char* vertPath, const char* fragPath);
    void Use();

    // Uniform setters
    void SetBool(const std::string &name, bool value);
    void SetInt(const std::string &name, int value);
    void SetFloat(const std::string &name, float value);
    void SetVec3(const std::string &name, const glm::vec3 &value);
    void SetMat4(const std::string &name, const glm::mat4 &mat);

    unsigned int GetID() const { return ID; }

private:
    unsigned int ID;
    std::unordered_map<std::string, int> uniformLocationCache;

    int GetUniformLocation(const std::string &name);
    void CheckCompileErrors(unsigned int shader, std::string type);
};