#pragma once
#include <SFML/Audio.hpp>
#include <unordered_map>
#include <string>
#include <vector>
#include <memory>
#include <glm/glm.hpp>

class AudioManager {
public:
    AudioManager();

    void LoadSound(const std::string& name, const std::string& path);
    void PlayGlobal(const std::string& name, float volume = 100.0f);
    void PlaySpatial(const std::string& name, glm::vec3 position, float volume = 100.0f, float attenuation = 10.0f);
    void UpdateListener(glm::vec3 position, glm::vec3 forward, glm::vec3 up);
    void PlayMusic(const std::string& path, float volume = 50.0f);
    void StopMusic();

private:
    std::unordered_map<std::string, sf::SoundBuffer> m_Buffers;
    std::vector<std::unique_ptr<sf::Sound>> m_Sounds;
    std::unique_ptr<sf::Music> m_Music;

    void CleanFinishedSounds();
};