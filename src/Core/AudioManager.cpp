#include "AudioManager.h"
#include <iostream>

AudioManager::AudioManager() {
    sf::Listener::setGlobalVolume(100.0f);
}

void AudioManager::LoadSound(const std::string& name, const std::string& path) {
    sf::SoundBuffer buffer;
    if (!buffer.loadFromFile(path)) {
        std::cerr << "ERROR: Failed to load sound: " << path << std::endl;
        return;
    }
    m_Buffers[name] = buffer;
}

// FIX: Implementation matches the header (accepts volume)
void AudioManager::PlayGlobal(const std::string& name, float volume) {
    CleanFinishedSounds();

    if (m_Buffers.find(name) == m_Buffers.end()) return;

    // Create sound from buffer
    auto sound = std::make_unique<sf::Sound>(m_Buffers[name]);

    sound->setVolume(volume);
    sound->setRelativeToListener(true); // Global sounds are always heard clearly
    sound->setPosition({0, 0, 0});
    sound->play();
    m_Sounds.push_back(std::move(sound));
}

// FIX: Implementation matches the header (accepts 3D params)
void AudioManager::PlaySpatial(const std::string& name, glm::vec3 position, float volume, float attenuation) {
    CleanFinishedSounds();

    if (m_Buffers.find(name) == m_Buffers.end()) return;

    auto sound = std::make_unique<sf::Sound>(m_Buffers[name]);

    sound->setVolume(volume);
    sound->setPosition({position.x, position.y, position.z});
    sound->setMinDistance(1.0f);
    sound->setAttenuation(attenuation);
    sound->play();
    m_Sounds.push_back(std::move(sound));
}

void AudioManager::UpdateListener(glm::vec3 position, glm::vec3 forward, glm::vec3 up) {
    sf::Listener::setPosition({position.x, position.y, position.z});
    sf::Listener::setDirection({forward.x, forward.y, forward.z});
    sf::Listener::setUpVector({up.x, up.y, up.z});
}

void AudioManager::PlayMusic(const std::string& path, float volume) {
    if (!m_Music) m_Music = std::make_unique<sf::Music>();

    if (m_Music->openFromFile(path)) {
        m_Music->setLooping(true);
        m_Music->setVolume(volume);
        m_Music->play();
    } else {
        std::cerr << "ERROR: Failed to load music: " << path << std::endl;
    }
}

void AudioManager::StopMusic() {
    if (m_Music) m_Music->stop();
}

void AudioManager::CleanFinishedSounds() {
    std::erase_if(m_Sounds, [](const auto& sound) {
        return sound->getStatus() == sf::Sound::Status::Stopped;
    });
}