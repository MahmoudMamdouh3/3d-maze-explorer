#pragma once
#include <unordered_map>
#include <string>
#include <SFML/Graphics.hpp>
#include <glad/glad.h>

class ResourceManager {
public:
    // Loads a texture or returns it if already loaded
    static unsigned int LoadTexture(const std::string& name, const std::string& path);

    // Get a stored texture ID
    static unsigned int GetTexture(const std::string& name);

    // Clean up all textures on shutdown
    static void Clear();

private:
    // Private constructor (Static Class)
    ResourceManager() {}

    // Cache: Name -> OpenGL Texture ID
    static std::unordered_map<std::string, unsigned int> textures;

    static unsigned int LoadTextureFromFile(const std::string& path);
};