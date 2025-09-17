#include "outputsystem.h"

#include <QDebug>
#include <QSettings>

#include <TalcsCore/MixerAudioSource.h>
#include <TalcsDevice/AudioDevice.h>
#include <TalcsDevice/AudioDriver.h>

#include <CoreApi/plugindatabase.h>

namespace Audio::Internal {

    Q_LOGGING_CATEGORY(audioOutputSystem, "diffscope.audio.outputSystem")

    OutputSystem::OutputSystem(QObject *parent) : QObject(parent), m_outputContext(new talcs::OutputContext) {
    }

    OutputSystem::~OutputSystem() = default;

    bool OutputSystem::initialize() {
        load();
        m_outputContext->setAdoptedBufferSize(m_adoptedBufferSize);
        m_outputContext->setAdoptedSampleRate(m_adoptedSampleRate);
        m_outputContext->controlMixer()->setGain(static_cast<float>(m_deviceGain));
        m_outputContext->controlMixer()->setPan(static_cast<float>(m_devicePan));
        m_outputContext->setHotPlugNotificationMode(m_hotPlugNotificationMode);
        // setFileBufferingReadAheadSize(AudioSettings::fileBufferingReadAheadSize());

        if (m_outputContext->initialize(m_driverName, m_deviceName)) {
            qCDebug(audioOutputSystem) << "Device initialized"
                     << m_outputContext->device()->name()
                     << m_outputContext->driver()->name()
                     << m_outputContext->adoptedBufferSize()
                     << m_outputContext->adoptedSampleRate();
            return true;
        } else {
            qCWarning(audioOutputSystem) << "Fatal: Cannot initialize";
            return false;
        }

    }
    bool OutputSystem::setDriver(const QString &driverName) {
        if (m_outputContext->setDriver(driverName)) {
            postSetDevice();
            qCDebug(audioOutputSystem) << "Driver changed"
                     << m_outputContext->device()->name()
                     << m_outputContext->driver()->name()
                     << m_outputContext->adoptedBufferSize()
                     << m_outputContext->adoptedSampleRate();
            return true;
        } else {
            qCWarning(audioOutputSystem) << "Fatal: cannot set driver" << driverName;
            return false;
        }
    }
    bool OutputSystem::setDevice(const QString &deviceName) {
        if (m_outputContext->setDevice(deviceName)) {
            postSetDevice();
            qCDebug(audioOutputSystem) << "Device changed"
                     << m_outputContext->device()->name()
                     << m_outputContext->driver()->name()
                     << m_outputContext->adoptedBufferSize()
                     << m_outputContext->adoptedSampleRate();
            return true;
        } else {
            qCWarning(audioOutputSystem) << "Fatal: Cannot set device"
                       << (m_outputContext->driver() ? m_outputContext->driver()->name() : "")
                       << deviceName;
            return false;
        }
    }

    bool OutputSystem::setAdoptedBufferSize(qint64 bufferSize) {
        m_adoptedBufferSize = bufferSize;
        if (m_outputContext->setAdoptedBufferSize(bufferSize)) {
            qCDebug(audioOutputSystem) << "Buffer size changed"
                     << m_outputContext->device()->name()
                     << m_outputContext->driver()->name()
                     << m_outputContext->adoptedBufferSize()
                     << m_outputContext->adoptedSampleRate();
            save();
            return true;
        } else {
            qCWarning(audioOutputSystem) << "Fatal: Cannot set buffer size"
                     << (m_outputContext->driver() ? m_outputContext->driver()->name() : "")
                     << (m_outputContext->device() ? m_outputContext->device()->name() : "")
                     << m_outputContext->adoptedBufferSize()
                     << m_outputContext->adoptedSampleRate();
            return false;
        }
    }

    bool OutputSystem::setAdoptedSampleRate(double sampleRate) {
        m_adoptedSampleRate = sampleRate;
        if (m_outputContext->setAdoptedSampleRate(sampleRate)) {
            qCDebug(audioOutputSystem) << "Sample rate changed"
                     << m_outputContext->device()->name()
                     << m_outputContext->driver()->name()
                     << m_outputContext->adoptedSampleRate()
                     << m_outputContext->adoptedSampleRate();
            save();
            return true;
        } else {
            qCWarning(audioOutputSystem) << "Fatal: Cannot set sample rate"
                       << (m_outputContext->driver() ? m_outputContext->driver()->name() : "")
                       << (m_outputContext->device() ? m_outputContext->device()->name() : "")
                       << m_outputContext->adoptedSampleRate()
                       << m_outputContext->adoptedSampleRate();
            return false;
        }
    }

    void OutputSystem::setHotPlugNotificationMode(talcs::OutputContext::HotPlugNotificationMode mode) {
        m_hotPlugNotificationMode = mode;
        m_outputContext->setHotPlugNotificationMode(mode);
        save();
        qCDebug(audioOutputSystem) << "Hot plug notification mode set to" << mode;
    }

    void OutputSystem::load() {
        auto settings = Core::PluginDatabase::settings();
        settings->beginGroup(staticMetaObject.className());
        m_adoptedBufferSize = settings->value("adoptedBufferSize").value<qint64>();
        m_adoptedSampleRate = settings->value("adoptedSampleRate").value<double>();
        m_deviceGain = settings->value("deviceGain", 1.0).toDouble();
        m_devicePan = settings->value("devicePan").toDouble();
        m_hotPlugNotificationMode = static_cast<talcs::OutputContext::HotPlugNotificationMode>(settings->value("hotPlugNotificationMode", talcs::OutputContext::HotPlugNotificationMode::Omni).toInt());
        m_driverName = settings->value("driverName").toString();
        m_deviceName = settings->value("deviceName").toString();
        settings->endGroup();
    }

    void OutputSystem::save() const {
        auto settings = Core::PluginDatabase::settings();
        settings->beginGroup(staticMetaObject.className());
        settings->setValue("adoptedBufferSize", m_adoptedBufferSize);
        settings->setValue("adoptedSampleRate", m_adoptedSampleRate);
        settings->setValue("deviceGain", m_deviceGain);
        settings->setValue("devicePan", m_devicePan);
        settings->setValue("hotPlugNotificationMode", m_hotPlugNotificationMode);
        settings->setValue("driverName", m_driverName);
        settings->setValue("deviceName", m_deviceName);
        settings->endGroup();
    }

    void OutputSystem::postSetDevice() {
        m_driverName = m_outputContext->driver()->name();
        m_deviceName = m_outputContext->device()->name();
        m_adoptedSampleRate = m_outputContext->adoptedSampleRate();
        m_adoptedBufferSize = m_outputContext->adoptedBufferSize();
        save();
    }
}
