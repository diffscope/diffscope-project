#include "GlobalAudioContext.h"
#include "GlobalAudioContext_p.h"

#include <audio/internal/AudioSystem.h>
#include <audio/internal/OutputSystem.h>

#include <TalcsDevice/OutputContext.h>
#include <TalcsFormat/FormatManager.h>
#include <TalcsWidgets/StandardFormatEntry.h>
#include <TalcsWidgets/WavpackFormatEntry.h>

namespace Audio {

    static GlobalAudioContext *m_instance{};

    static talcs::OutputContext *outputContext() {
        return Internal::AudioSystem::outputSystem()->outputContext();
    }

    GlobalAudioContext::GlobalAudioContext(QObject *parent)
        : QObject(parent), d_ptr(new GlobalAudioContextPrivate) {
        Q_ASSERT(!m_instance);
        m_instance = this;

        Q_D(GlobalAudioContext);
        d->q_ptr = this;
        d->formatManager = std::make_unique<talcs::FormatManager>();
        d->formatManager->addEntry(new talcs::StandardFormatEntry);
        d->formatManager->addEntry(new talcs::WavpackFormatEntry);

        auto context = outputContext();
        connect(context, &talcs::OutputContext::deviceChanged, this, &GlobalAudioContext::deviceChanged);
        connect(context, &talcs::OutputContext::bufferSizeChanged, this, &GlobalAudioContext::bufferSizeChanged);
        connect(context, &talcs::OutputContext::sampleRateChanged, this, &GlobalAudioContext::sampleRateChanged);
    }

    GlobalAudioContext::~GlobalAudioContext() {
        m_instance = nullptr;
    }

    GlobalAudioContext *GlobalAudioContext::instance() {
        return m_instance;
    }

    talcs::AudioDriverManager *GlobalAudioContext::driverManager() {
        return outputContext()->driverManager();
    }

    talcs::AudioDriver *GlobalAudioContext::driver() {
        return outputContext()->driver();
    }

    talcs::AudioDevice *GlobalAudioContext::device() {
        return outputContext()->device();
    }

    talcs::AudioSourcePlayback *GlobalAudioContext::playback() {
        return outputContext()->playback();
    }

    talcs::MixerAudioSource *GlobalAudioContext::controlMixer() {
        return outputContext()->controlMixer();
    }

    talcs::MixerAudioSource *GlobalAudioContext::preMixer() {
        return outputContext()->preMixer();
    }

    qint64 GlobalAudioContext::bufferSize() {
        return outputContext()->adoptedBufferSize();
    }

    double GlobalAudioContext::sampleRate() {
        return outputContext()->adoptedSampleRate();
    }

    talcs::FormatManager *GlobalAudioContext::formatManager() {
        Q_ASSERT(m_instance);
        auto d = m_instance->d_func();
        return d->formatManager.get();
    }

    GlobalAudioContext *GlobalAudioContextPrivate::create(QObject *parent) {
        Q_ASSERT(!m_instance);
        return new GlobalAudioContext(parent);
    }

}
