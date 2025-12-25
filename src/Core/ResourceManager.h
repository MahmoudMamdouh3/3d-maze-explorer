#pragma once
#include <unordered_map>
#include <string>
#include <SFML/Graphics.hpp>
#include <glad/glad.h>

class ResourceManager {
public:

    static unsigned int LoadTexture(const std::string& name, const std::string& path);


    static unsigned int GetTexture(const std::string& name);


    static void Clear();

private:

    ResourceManager() {}


    static std::unordered_map<std::string, unsigned int> textures;

    static unsigned int LoadTextureFromFile(const std::string& path);
};