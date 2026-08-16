// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "GlobalAudioContext.h"
#include "GlobalAudioContext_p.h"

#include <QSettings>

#include <CoreApi/runtimeinterface.h>

#include <audio/internal/AudioSystem.h>
#include <audio/internal/OutputSystem.h>

#include <TalcsCore/MixerAudioSource.h>
#include <TalcsDevice/OutputContext.h>
#include <TalcsFormat/FormatManager.h>
#include <TalcsWidgets/StandardFormatEntry.h>
#include <TalcsWidgets/AACFormatEntry.h>
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
#if defined(Q_OS_WIN) || defined(Q_OS_MAC)
        d->formatManager->addEntry(new talcs::AACFormatEntry);
#endif
        d->formatManager->addEntry(new talcs::WavpackFormatEntry);

        auto settings = Core::RuntimeInterface::settings();
        settings->beginGroup(staticMetaObject.className());
        d->metronomeEnabled = settings->value(QStringLiteral("metronomeEnabled"), false).toBool();
        d->metronomeGain = settings->value(QStringLiteral("metronomeGain"), 1.0).toDouble();
        d->metronomePan = settings->value(QStringLiteral("metronomePan"), 0.0).toDouble();
        const bool hasDeviceGain = settings->contains(QStringLiteral("deviceGain"));
        const bool hasDevicePan = settings->contains(QStringLiteral("devicePan"));
        d->deviceGain = settings->value(QStringLiteral("deviceGain"), 1.0).toDouble();
        d->devicePan = settings->value(QStringLiteral("devicePan"), 0.0).toDouble();
        settings->endGroup();

        if (!hasDeviceGain || !hasDevicePan) {
            settings->beginGroup(Internal::OutputSystem::staticMetaObject.className());
            if (!hasDeviceGain) {
                d->deviceGain = settings->value(QStringLiteral("deviceGain"), 1.0).toDouble();
            }
            if (!hasDevicePan) {
                d->devicePan = settings->value(QStringLiteral("devicePan"), 0.0).toDouble();
            }
            settings->endGroup();

            settings->beginGroup(staticMetaObject.className());
            settings->setValue(QStringLiteral("deviceGain"), d->deviceGain);
            settings->setValue(QStringLiteral("devicePan"), d->devicePan);
            settings->endGroup();
        }

        outputContext()->controlMixer()->setGain(static_cast<float>(d->deviceGain));
        outputContext()->controlMixer()->setPan(static_cast<float>(d->devicePan));

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

    bool GlobalAudioContext::metronomeEnabled() {
        Q_ASSERT(m_instance);
        auto d = m_instance->d_func();
        QMutexLocker locker(&d->propertiesMutex);
        return d->metronomeEnabled;
    }

    void GlobalAudioContext::setMetronomeEnabled(bool enabled) {
        Q_ASSERT(m_instance);
        auto d = m_instance->d_func();
        {
            QMutexLocker locker(&d->propertiesMutex);
            if (d->metronomeEnabled == enabled) {
                return;
            }
            d->metronomeEnabled = enabled;
        }

        auto settings = Core::RuntimeInterface::settings();
        settings->beginGroup(staticMetaObject.className());
        settings->setValue(QStringLiteral("metronomeEnabled"), enabled);
        settings->endGroup();
        Q_EMIT m_instance->metronomeEnabledChanged(enabled);
    }

    double GlobalAudioContext::metronomeGain() {
        Q_ASSERT(m_instance);
        auto d = m_instance->d_func();
        QMutexLocker locker(&d->propertiesMutex);
        return d->metronomeGain;
    }

    void GlobalAudioContext::setMetronomeGain(double gain) {
        Q_ASSERT(m_instance);
        auto d = m_instance->d_func();
        {
            QMutexLocker locker(&d->propertiesMutex);
            if (qFuzzyCompare(d->metronomeGain, gain)) {
                return;
            }
            d->metronomeGain = gain;
        }

        auto settings = Core::RuntimeInterface::settings();
        settings->beginGroup(staticMetaObject.className());
        settings->setValue(QStringLiteral("metronomeGain"), gain);
        settings->endGroup();
        Q_EMIT m_instance->metronomeGainChanged(gain);
    }

    double GlobalAudioContext::metronomePan() {
        Q_ASSERT(m_instance);
        auto d = m_instance->d_func();
        QMutexLocker locker(&d->propertiesMutex);
        return d->metronomePan;
    }

    void GlobalAudioContext::setMetronomePan(double pan) {
        Q_ASSERT(m_instance);
        auto d = m_instance->d_func();
        {
            QMutexLocker locker(&d->propertiesMutex);
            if (qFuzzyCompare(d->metronomePan, pan)) {
                return;
            }
            d->metronomePan = pan;
        }

        auto settings = Core::RuntimeInterface::settings();
        settings->beginGroup(staticMetaObject.className());
        settings->setValue(QStringLiteral("metronomePan"), pan);
        settings->endGroup();
        Q_EMIT m_instance->metronomePanChanged(pan);
    }

    double GlobalAudioContext::deviceGain() {
        Q_ASSERT(m_instance);
        auto d = m_instance->d_func();
        QMutexLocker locker(&d->propertiesMutex);
        return d->deviceGain;
    }

    void GlobalAudioContext::setDeviceGain(double gain) {
        Q_ASSERT(m_instance);
        auto d = m_instance->d_func();
        {
            QMutexLocker locker(&d->propertiesMutex);
            if (qFuzzyCompare(d->deviceGain, gain)) {
                return;
            }
            d->deviceGain = gain;
        }

        outputContext()->controlMixer()->setGain(static_cast<float>(gain));
        auto settings = Core::RuntimeInterface::settings();
        settings->beginGroup(staticMetaObject.className());
        settings->setValue(QStringLiteral("deviceGain"), gain);
        settings->endGroup();
        Q_EMIT m_instance->deviceGainChanged(gain);
    }

    double GlobalAudioContext::devicePan() {
        Q_ASSERT(m_instance);
        auto d = m_instance->d_func();
        QMutexLocker locker(&d->propertiesMutex);
        return d->devicePan;
    }

    void GlobalAudioContext::setDevicePan(double pan) {
        Q_ASSERT(m_instance);
        auto d = m_instance->d_func();
        {
            QMutexLocker locker(&d->propertiesMutex);
            if (qFuzzyCompare(d->devicePan, pan)) {
                return;
            }
            d->devicePan = pan;
        }

        outputContext()->controlMixer()->setPan(static_cast<float>(pan));
        auto settings = Core::RuntimeInterface::settings();
        settings->beginGroup(staticMetaObject.className());
        settings->setValue(QStringLiteral("devicePan"), pan);
        settings->endGroup();
        Q_EMIT m_instance->devicePanChanged(pan);
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
