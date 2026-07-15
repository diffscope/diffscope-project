#include "AudioSystem.h"

#include <audio/internal/OutputSystem.h>
#include <audio/internal/WaveformSingerMetadata.h>

#include <coreplugin/CoreInterface.h>

namespace Audio::Internal {

    AudioSystem *m_instance{};

    AudioSystem::AudioSystem(QObject *parent) : QObject(parent) {
        Q_ASSERT(!m_instance);
        m_instance = this;
        m_outputSystem = new OutputSystem(this);
        m_singerRegistrationSuccessful = WaveformSingerMetadata::registerAll(Core::CoreInterface::singerRegistry());
    }

    AudioSystem::~AudioSystem() {
        m_instance = nullptr;
    }

    AudioSystem *AudioSystem::instance() {
        return m_instance;
    }

    OutputSystem *AudioSystem::outputSystem() {
        Q_ASSERT(m_instance);
        return m_instance->m_outputSystem;
    }

    bool AudioSystem::isSingerRegistrationSuccessful() {
        Q_ASSERT(m_instance);
        return m_instance->m_singerRegistrationSuccessful;
    }
}
