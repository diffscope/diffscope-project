#include "AudioPreference.h"

#include <QSettings>

#include <CoreApi/runtimeinterface.h>

namespace Audio::Internal {

    class AudioPreferencePrivate {
    public:
        AudioPreference::PlayheadBehavior playbackBehavior{};
    };

    static AudioPreference *m_instance = nullptr;

    AudioPreference::AudioPreference(QObject *parent) : QObject(parent), d_ptr(new AudioPreferencePrivate) {
        Q_ASSERT(!m_instance);
        m_instance = this;
    }

    AudioPreference::~AudioPreference() {
        m_instance = nullptr;
    }

    AudioPreference *AudioPreference::instance() {
        return m_instance;
    }

    void AudioPreference::load() {
        Q_D(AudioPreference);
        auto settings = Core::RuntimeInterface::settings();
        settings->beginGroup(staticMetaObject.className());
        d->playbackBehavior = settings->value("playbackBehavior", QVariant::fromValue(PB_ReturnToStart)).value<PlayheadBehavior>();
        emit playbackBehaviorChanged();
        settings->endGroup();
    }

    void AudioPreference::save() const {
        Q_D(const AudioPreference);
        auto settings = Core::RuntimeInterface::settings();
        settings->beginGroup(staticMetaObject.className());
        settings->setValue("playbackBehavior", d->playbackBehavior);
        settings->endGroup();
    }

#define M_INSTANCE_D      \
    Q_ASSERT(m_instance); \
    auto d = m_instance->d_func()

    AudioPreference::PlayheadBehavior AudioPreference::playbackBehavior() {
        M_INSTANCE_D;
        return d->playbackBehavior;
    }

    void AudioPreference::setPlaybackBehavior(PlayheadBehavior playbackBehavior) {
        M_INSTANCE_D;
        if (d->playbackBehavior == playbackBehavior)
            return;
        d->playbackBehavior = playbackBehavior;
        emit m_instance->playbackBehaviorChanged();
    }

}
