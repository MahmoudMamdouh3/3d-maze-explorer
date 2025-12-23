#include "ResourceManager.h"
#include <iostream>

// Define the static member
std::unordered_map<std::string, unsigned int> ResourceManager::textures;

unsigned int ResourceManager::LoadTexture(const std::string& name, const std::string& path) {
    // 1. Check if already loaded
    if (textures.find(name) != textures.end()) {
        return textures[name];
    }

    // 2. Load and store
    unsigned int id = LoadTextureFromFile(path);
    textures[name] = id;
    return id;
}

unsigned int ResourceManager::GetTexture(const std::string& name) {
    if (textures.find(name) != textures.end()) {
        return textures[name];
    }
    std::cerr << "WARNING: ResourceManager could not find texture: " << name << std::endl;
    return 0;
}

void ResourceManager::Clear() {
    for (auto& iter : textures) {
        glDeleteTextures(1, &iter.second);
    }
    textures.clear();
}

unsigned int ResourceManager::LoadTextureFromFile(const std::string& path) {
    sf::Image image;
    if (!image.loadFromFile(path)) {
        std::cerr << "ERROR: Failed to load texture: " << path << std::endl;
        return 0;
    }

    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    // Send SFML image data to OpenGL
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
                 static_cast<int>(image.getSize().x),
                 static_cast<int>(image.getSize().y),
                 0, GL_RGBA, GL_UNSIGNED_BYTE, image.getPixelsPtr());

    glGenerateMipmap(GL_TEXTURE_2D);

    // Set Wrapping and Filtering (Trilinear filtering for AAA quality)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    return textureID;
}