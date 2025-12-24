#include "AudioManager.h"
#include <iostream>
#include <SFML/Audio.hpp> // Ensure full inclusion

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

void AudioManager::PlayGlobal(const std::string& name, float volume) {
    CleanFinishedSounds();

    // SFML 3 / C++20 Fix: Use contains()
    if (!m_Buffers.contains(name)) return;

    // AUDIO POLISH: Prevent Footstep Overlap
    if (name == "footstep") {
        for (const auto& s : m_Sounds) {
            // SFML 3 FIX: Use sf::SoundSource::Status::Playing
            if (s->getStatus() == sf::SoundSource::Status::Playing && s->getVolume() > 0.0f) {
                // If a footstep is already playing loudly, don't spam another one
                return;
            }
        }
    }

    auto sound = std::make_unique<sf::Sound>(m_Buffers[name]);
    sound->setVolume(volume);
    sound->setRelativeToListener(true);
    sound->setPosition({0, 0, 0});
    sound->play();
    m_Sounds.push_back(std::move(sound));
}

void AudioManager::PlaySpatial(const std::string& name, glm::vec3 position, float volume, float attenuation) {
    CleanFinishedSounds();

    if (!m_Buffers.contains(name)) return; // C++20 Fix

    auto sound = std::make_unique<sf::Sound>(m_Buffers[name]);
    sound->setVolume(volume);
    sound->setPosition({position.x, position.y, position.z});
    sound->setMinDistance(1.0f);
    sound->setAttenuation(attenuation);
    sound->play();
    m_Sounds.push_back(std::move(sound));
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

// NEW: Cuts all audio immediately (for Death/Win screens)
void AudioManager::StopAllSounds() {
    for (auto& sound : m_Sounds) {
        sound->stop();
    }
    m_Sounds.clear();
    StopMusic();
}

void AudioManager::UpdateListener(glm::vec3 position, glm::vec3 forward, glm::vec3 up) {
    sf::Listener::setPosition({position.x, position.y, position.z});
    sf::Listener::setDirection({forward.x, forward.y, forward.z});
    sf::Listener::setUpVector({up.x, up.y, up.z});
}

void AudioManager::CleanFinishedSounds() {
    std::erase_if(m_Sounds, [](const auto& sound) {
        // SFML 3 FIX: Use sf::SoundSource::Status::Stopped
        return sound->getStatus() == sf::SoundSource::Status::Stopped;
    });
}