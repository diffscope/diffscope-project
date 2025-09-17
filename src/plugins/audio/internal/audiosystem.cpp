#include "audiosystem.h"

#include <audio/internal/outputsystem.h>

namespace Audio::Internal {

    AudioSystem *m_instance{};

    AudioSystem::AudioSystem(QObject *parent) : QObject(parent) {
        Q_ASSERT(!m_instance);
        m_instance = this;
        m_outputSystem = new OutputSystem(this);
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
}
