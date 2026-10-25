// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "OutputSystem.h"

#include <QDebug>
#include <QSettings>

#include <CoreApi/runtimeinterface.h>

#include <TalcsCore/MixerAudioSource.h>
#include <TalcsDevice/AudioDevice.h>
#include <TalcsDevice/AudioDriver.h>

#include <audio/internal/DeviceTester.h>

namespace Audio::Internal {

    Q_LOGGING_CATEGORY(audioOutputSystem, "diffscope.audio.outputSystem")

    OutputSystem::OutputSystem(QObject *parent) : QObject(parent), m_outputContext(new talcs::OutputContext), m_deviceTester(new DeviceTester) {
    }

    OutputSystem::~OutputSystem() {
        qCInfo(audioOutputSystem) << "Audio output is shutting down.";
        m_outputContext->preMixer()->removeSource(m_deviceTester.get());
    }

    bool OutputSystem::initialize() {
        load();
        qCInfo(audioOutputSystem) << "Audio output initialization started."
                                 << "driver:" << m_driverName << "device:" << m_deviceName
                                 << "buffer size:" << m_adoptedBufferSize << "sample rate:" << m_adoptedSampleRate
                                 << "hot plug notification mode:" << m_hotPlugNotificationMode;
        m_outputContext->setAdoptedBufferSize(m_adoptedBufferSize);
        m_outputContext->setAdoptedSampleRate(m_adoptedSampleRate);
        m_outputContext->setHotPlugNotificationMode(m_hotPlugNotificationMode);
        // setFileBufferingReadAheadSize(AudioSettings::fileBufferingReadAheadSize());

        m_outputContext->preMixer()->addSource(m_deviceTester.get());

        if (m_outputContext->initialize(m_driverName, m_deviceName)) {
            postSetDevice();
            qCInfo(audioOutputSystem) << "Audio output initialization completed.";
            logOutputInfo();
            return true;
        } else {
            qCCritical(audioOutputSystem) << "Audio output initialization failed."
                                         << "requested driver:" << m_driverName << "requested device:" << m_deviceName;
            logOutputInfo();
            return false;
        }
    }
    talcs::OutputContext *OutputSystem::outputContext() const {
        return m_outputContext.get();
    }
    bool OutputSystem::setDriver(const QString &driverName) {
        qCInfo(audioOutputSystem) << "Audio driver change requested." << "driver:" << driverName;
        if (m_outputContext->setDriver(driverName, talcs::OutputContext::DO_UsePreferredSpec)) {
            postSetDevice();
            qCInfo(audioOutputSystem) << "Audio driver change completed." << "driver:" << driverName;
            logOutputInfo();
            return true;
        } else {
            qCWarning(audioOutputSystem) << "Audio driver change failed." << "driver:" << driverName;
            logOutputInfo();
            return false;
        }
    }
    bool OutputSystem::setDevice(const QString &deviceName) {
        qCInfo(audioOutputSystem) << "Audio device change requested." << "device:" << deviceName;
        if (m_outputContext->setDevice(deviceName, talcs::OutputContext::DO_UsePreferredSpec)) {
            postSetDevice();
            qCInfo(audioOutputSystem) << "Audio device change completed." << "device:" << deviceName;
            logOutputInfo();
            return true;
        } else {
            qCWarning(audioOutputSystem) << "Audio device change failed." << "device:" << deviceName;
            logOutputInfo();
            return false;
        }
    }

    bool OutputSystem::setAdoptedBufferSize(qint64 bufferSize) {
        qCInfo(audioOutputSystem) << "Audio buffer size change requested." << "buffer size:" << bufferSize;
        m_adoptedBufferSize = bufferSize;
        if (m_outputContext->setAdoptedBufferSize(bufferSize)) {
            qCInfo(audioOutputSystem) << "Audio buffer size change completed." << "buffer size:" << bufferSize;
            logOutputInfo();
            save();
            return true;
        } else {
            qCWarning(audioOutputSystem) << "Audio buffer size change failed." << "buffer size:" << bufferSize;
            logOutputInfo();
            return false;
        }
    }

    bool OutputSystem::setAdoptedSampleRate(double sampleRate) {
        qCInfo(audioOutputSystem) << "Audio sample rate change requested." << "sample rate:" << sampleRate;
        m_adoptedSampleRate = sampleRate;
        if (m_outputContext->setAdoptedSampleRate(sampleRate)) {
            qCInfo(audioOutputSystem) << "Audio sample rate change completed." << "sample rate:" << sampleRate;
            logOutputInfo();
            save();
            return true;
        } else {
            qCWarning(audioOutputSystem) << "Audio sample rate change failed." << "sample rate:" << sampleRate;
            logOutputInfo();
            return false;
        }
    }

    void OutputSystem::setHotPlugNotificationMode(
        talcs::OutputContext::HotPlugNotificationMode mode
    ) {
        if (m_hotPlugNotificationMode == mode) {
            return;
        }
        m_hotPlugNotificationMode = mode;
        m_outputContext->setHotPlugNotificationMode(mode);
        save();
        qCInfo(audioOutputSystem) << "Hot plug notification mode changed." << "mode:" << mode;
        Q_EMIT hotPlugNotificationModeChanged();
    }

    bool OutputSystem::isReady() const {
        return m_outputContext->device() && m_outputContext->device()->isOpen();
    }

    void OutputSystem::playTestSound() {
        qCInfo(audioOutputSystem) << "Audio test sound requested." << "output ready:" << isReady();
        if (!isReady()) {
            qCWarning(audioOutputSystem) << "The audio test sound cannot be played because the output device is not open.";
        }
        logOutputInfo();
        m_deviceTester->playTestSound();
    }

    void OutputSystem::load() {
        auto settings = Core::RuntimeInterface::settings();
        settings->beginGroup(staticMetaObject.className());
        m_adoptedBufferSize = settings->value("adoptedBufferSize").value<qint64>();
        m_adoptedSampleRate = settings->value("adoptedSampleRate").value<double>();
        m_hotPlugNotificationMode = static_cast<talcs::OutputContext::HotPlugNotificationMode>(settings->value("hotPlugNotificationMode", talcs::OutputContext::HotPlugNotificationMode::Omni).toInt());
        m_driverName = settings->value("driverName").toString();
        m_deviceName = settings->value("deviceName").toString();
        settings->endGroup();
        qCDebug(audioOutputSystem) << "Audio output settings loaded." << "driver:" << m_driverName << "device:" << m_deviceName;
    }

    void OutputSystem::save() const {
        auto settings = Core::RuntimeInterface::settings();
        settings->beginGroup(staticMetaObject.className());
        settings->setValue("adoptedBufferSize", m_adoptedBufferSize);
        settings->setValue("adoptedSampleRate", m_adoptedSampleRate);
        settings->setValue("hotPlugNotificationMode", m_hotPlugNotificationMode);
        settings->setValue("driverName", m_driverName);
        settings->setValue("deviceName", m_deviceName);
        settings->endGroup();
        qCDebug(audioOutputSystem) << "Audio output settings saved." << "driver:" << m_driverName << "device:" << m_deviceName
                                  << "buffer size:" << m_adoptedBufferSize << "sample rate:" << m_adoptedSampleRate;
    }

    void OutputSystem::postSetDevice() {
        m_driverName = m_outputContext->driver()->name();
        m_deviceName = m_outputContext->device()->name();
        m_adoptedSampleRate = m_outputContext->adoptedSampleRate();
        m_adoptedBufferSize = m_outputContext->adoptedBufferSize();
        save();
    }

    void OutputSystem::logOutputInfo() const {
        const auto *driver = m_outputContext->driver();
        const auto *device = m_outputContext->device();
        qCInfo(audioOutputSystem) << "Audio output state recorded."
                                 << "driver:" << (driver ? driver->name() : QString())
                                 << "driver display name:" << (driver ? driver->displayName() : QString())
                                 << "device:" << (device ? device->name() : QString())
                                 << "sample rate:" << m_outputContext->adoptedSampleRate()
                                 << "buffer size:" << m_outputContext->adoptedBufferSize()
                                 << "open:" << (device && device->isOpen())
                                 << "started:" << (device && device->isStarted());
    }
}
