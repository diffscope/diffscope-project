// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "MidiInputContext.h"
#include "MidiInputContext_p.h"

#include <algorithm>
#include <cmath>

#include <QGuiApplication>
#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QMetaObject>
#include <QSettings>
#include <QTimer>
#include <QVariantMap>

#include <CoreApi/runtimeinterface.h>
#include <CoreApi/windowsystem.h>

#include <TalcsCore/MixerAudioSource.h>

#include <coreplugin/CoreInterface.h>
#include <coreplugin/ProjectWindowInterface.h>

#include <audio/GlobalAudioContext.h>
#include <audio/internal/AudioSystem.h>
#include <audio/internal/OutputSystem.h>
#include <audio/internal/WaveformTypeCatalog.h>

namespace Audio {

    MidiInputContextPrivate::ControlListener::ControlListener(MidiInputContextPrivate *context) : m_context(context) {
    }

    bool MidiInputContextPrivate::ControlListener::processEvent(const talcs::MidiInputEvent &event) {
        if (event.type == talcs::MidiInputEvent::Message && m_context->mmcEnabled.load() && event.message.isMidiMachineControlMessage()) {
            QMetaObject::invokeMethod(m_context->q_ptr, [context = m_context, event] {
                context->dispatchControl(event);
            }, Qt::QueuedConnection);
        }
        return false;
    }

    void MidiInputContextPrivate::ControlListener::processError(const QString &error) {
        QMetaObject::invokeMethod(m_context->q_ptr, [context = m_context, error] {
            if (context->device && context->device->errorString() == error) {
                context->error = error;
                context->device.reset();
                Q_EMIT context->q_ptr->deviceChanged();
            }
        }, Qt::QueuedConnection);
    }

    MidiInputContext::MidiInputContext(QObject *parent) : QObject(parent), d_ptr(new MidiInputContextPrivate) {
        Q_D(MidiInputContext);
        d->q_ptr = this;
        d->dispatcher.addFilter(&d->controlListener);
        d->dispatcher.addFilter(&d->integrator);
        d->mixer.addStream(&d->preview);
        d->integrator.setStream(&d->mixer);
        auto settings = Core::RuntimeInterface::settings();
        settings->beginGroup(staticMetaObject.className());
        d->deviceId = settings->value(QStringLiteral("deviceId")).toString();
        d->previewEnabled = settings->value(QStringLiteral("previewEnabled"), true).toBool();
        d->previewType = Internal::WaveformTypeCatalog::normalizedType(settings->value(QStringLiteral("previewType"), Internal::WaveformTypeCatalog::fallbackType()).toString());
        d->previewGain = std::clamp(settings->value(QStringLiteral("previewGain"), 1.0).toDouble(), 0.0, 2.0);
        if (!std::isfinite(d->previewGain))
            d->previewGain = 1.0;
        d->mmcEnabled.store(settings->value(QStringLiteral("mmcEnabled"), true).toBool());
        settings->endGroup();
        d->preview.configure(d->previewEnabled, Internal::WaveformTypeCatalog::indexOf(d->previewType), d->previewGain);
    }

    MidiInputContext::~MidiInputContext() {
        Q_D(MidiInputContext);
        if (d->refreshTimer)
            d->refreshTimer->stop();
        d->device.reset();
        if (d->sourceRegistered)
            GlobalAudioContext::preMixer()->removeSource(&d->integrator);
        d->integrator.close();
        d->integrator.setStream(nullptr);
        d->mixer.removeStream(&d->preview);
        d->dispatcher.removeFilter(&d->integrator);
        d->dispatcher.removeFilter(&d->controlListener);
    }

    MidiInputContext *MidiInputContext::instance() {
        return Core::RuntimeInterface::instance()->getFirstObject<MidiInputContext>();
    }

    void MidiInputContext::initialize() {
        Q_D(MidiInputContext);
        if (d->initialized)
            return;
        d->initialized = true;
        d->sourceRegistered = GlobalAudioContext::preMixer()->addSource(&d->integrator, false);
        refreshDevices();
        d->refreshTimer = new QTimer(this);
        d->refreshTimer->setInterval(2000);
        connect(d->refreshTimer, &QTimer::timeout, this, &MidiInputContext::refreshDevices);
        d->refreshTimer->start();
    }

    QVariantList MidiInputContext::devices() const {
        Q_D(const MidiInputContext);
        return d->devices;
    }

    QString MidiInputContext::deviceName() const {
        Q_D(const MidiInputContext);
        for (const auto &entry : d->devices) {
            const auto device = entry.toMap();
            if (device.value(QStringLiteral("value")).toString() == d->deviceId)
                return device.value(QStringLiteral("name")).toString();
        }
        const auto identity = QJsonDocument::fromJson(d->deviceId.toUtf8()).array();
        return identity.isEmpty() ? QString() : identity.first().toString();
    }

    bool MidiInputContext::isDeviceReady() const {
        Q_D(const MidiInputContext);
        return d->device && d->device->isOpen() && d->device->errorString().isEmpty();
    }

    QString MidiInputContext::deviceErrorString() const {
        Q_D(const MidiInputContext);
        return d->error;
    }

    QString MidiInputContext::deviceId() const {
        Q_D(const MidiInputContext);
        return d->deviceId;
    }

    bool MidiInputContext::previewEnabled() const {
        Q_D(const MidiInputContext);
        return d->previewEnabled;
    }

    QString MidiInputContext::previewType() const {
        Q_D(const MidiInputContext);
        return d->previewType;
    }

    double MidiInputContext::previewGain() const {
        Q_D(const MidiInputContext);
        return d->previewGain;
    }

    bool MidiInputContext::mmcEnabled() const {
        Q_D(const MidiInputContext);
        return d->mmcEnabled.load();
    }

    bool MidiInputContext::setDeviceId(const QString &deviceId) {
        Q_D(MidiInputContext);
        if (d->deviceId == deviceId && (deviceId.isEmpty() || isDeviceReady()))
            return true;
        if (deviceId.isEmpty()) {
            d->device.reset();
            d->deviceId.clear();
            d->error.clear();
        } else {
            const auto index = d->portIds.indexOf(deviceId);
            if (index < 0) {
                d->error = tr("The MIDI input device is unavailable.");
                Q_EMIT deviceChanged();
                return false;
            }
            auto candidate = std::make_unique<talcs::MidiInputDevice>(index);
            candidate->listener()->addFilter(&d->dispatcher);
            if (!candidate->open()) {
                d->error = candidate->errorString();
                Q_EMIT deviceChanged();
                return false;
            }
            d->device = std::move(candidate);
            d->deviceId = deviceId;
            d->error.clear();
        }
        d->save();
        Q_EMIT deviceChanged();
        return true;
    }

    void MidiInputContext::setPreviewEnabled(bool enabled) {
        Q_D(MidiInputContext);
        if (d->previewEnabled == enabled)
            return;
        d->previewEnabled = enabled;
        d->updatePreview();
        d->save();
        Q_EMIT previewEnabledChanged();
    }

    void MidiInputContext::setPreviewType(const QString &type) {
        Q_D(MidiInputContext);
        const auto normalized = Internal::WaveformTypeCatalog::normalizedType(type);
        if (d->previewType == normalized)
            return;
        d->previewType = normalized;
        d->updatePreview();
        d->save();
        Q_EMIT previewTypeChanged();
    }

    void MidiInputContext::setPreviewGain(double gain) {
        Q_D(MidiInputContext);
        if (!std::isfinite(gain))
            return;
        gain = std::clamp(gain, 0.0, 2.0);
        if (d->previewGain == gain)
            return;
        d->previewGain = gain;
        d->updatePreview();
        d->save();
        Q_EMIT previewGainChanged();
    }

    void MidiInputContext::setMmcEnabled(bool enabled) {
        Q_D(MidiInputContext);
        if (d->mmcEnabled.exchange(enabled) == enabled)
            return;
        d->save();
        Q_EMIT mmcEnabledChanged();
    }

    bool MidiInputContext::playPreviewTest(bool alternative) {
        Q_D(MidiInputContext);
        stopPreviewTest();
        if (!d->previewEnabled || !d->preview.isOpen() || !Internal::AudioSystem::outputSystem()->isReady())
            return false;
        d->preview.startTest(alternative);
        return true;
    }

    void MidiInputContext::stopPreviewTest() {
        Q_D(MidiInputContext);
        d->preview.stopTest();
    }

    bool MidiInputContext::isPreviewTestPlaying() const {
        Q_D(const MidiInputContext);
        return Internal::AudioSystem::outputSystem()->isReady() && d->preview.isTestPlaying();
    }

    void MidiInputContext::refreshDevices() {
        Q_D(MidiInputContext);
        const auto names = talcs::MidiInputDevice::devices();
        if (d->portNames != names) {
            d->portNames = names;
            d->portIds.clear();
            d->devices.clear();
            QHash<QString, int> occurrences;
            for (const auto &name : names) {
                const int occurrence = occurrences[name]++;
                const auto id = QString::fromUtf8(QJsonDocument(QJsonArray{name, occurrence}).toJson(QJsonDocument::Compact));
                d->portIds.append(id);
                d->devices.append(QVariantMap{
                    {QStringLiteral("value"), id},
                    {QStringLiteral("name"), names.count(name) > 1 ? tr("%1 (Port %L2)").arg(name).arg(occurrence + 1) : name},
                });
            }
            if (d->device && !d->portIds.contains(d->deviceId)) {
                d->device.reset();
                Q_EMIT deviceChanged();
            }
            Q_EMIT devicesChanged();
        }
        d->reconnectDevice();
    }

    void MidiInputContext::interruptPreview() {
        Q_D(MidiInputContext);
        stopPreviewTest();
        d->integrator.flush();
    }

    talcs::MidiMessageListener *MidiInputContext::listener() const {
        Q_D(const MidiInputContext);
        return &d->dispatcher;
    }

    void MidiInputContext::submitEvent(const talcs::MidiInputEvent &event) {
        Q_D(MidiInputContext);
        d->dispatcher.eventCallback(event);
    }

    void MidiInputContextPrivate::reconnectDevice() {
        Q_Q(MidiInputContext);
        if (!deviceId.isEmpty() && portIds.contains(deviceId) && !q->isDeviceReady())
            q->setDeviceId(deviceId);
    }

    void MidiInputContextPrivate::updatePreview() {
        const auto type = Internal::WaveformTypeCatalog::indexOf(previewType);
        if (preview.previewNeedsReset(previewEnabled, type)) {
            preview.stopTest();
            integrator.flush();
        }
        preview.configure(previewEnabled, type, previewGain);
    }

    void MidiInputContextPrivate::dispatchControl(const talcs::MidiInputEvent &event) {
        if (!mmcEnabled.load() || !event.message.isMidiMachineControlMessage() || event.message.getRawDataSize() != 6)
            return;
        if (QGuiApplication::applicationState() != Qt::ApplicationActive)
            return;
        auto window = qobject_cast<Core::ProjectWindowInterface *>(Core::CoreInterface::windowSystem()->findWindow(QGuiApplication::focusWindow()));
        if (!window || window->isEffectivelyClosed())
            return;
        QString action;
        switch (event.message.getMidiMachineControlCommand()) {
        case talcs::MidiMessage::mmc_play:
        case talcs::MidiMessage::mmc_deferredplay:
            action = QStringLiteral("org.diffscope.audio.playback.play");
            break;
        case talcs::MidiMessage::mmc_pause:
            action = QStringLiteral("org.diffscope.audio.playback.pause");
            break;
        case talcs::MidiMessage::mmc_stop:
            action = QStringLiteral("org.diffscope.audio.playback.stop");
            break;
        default:
            return;
        }
        window->triggerAction(action);
    }

    void MidiInputContextPrivate::save() const {
        auto settings = Core::RuntimeInterface::settings();
        settings->beginGroup(MidiInputContext::staticMetaObject.className());
        settings->setValue(QStringLiteral("deviceId"), deviceId);
        settings->setValue(QStringLiteral("previewEnabled"), previewEnabled);
        settings->setValue(QStringLiteral("previewType"), previewType);
        settings->setValue(QStringLiteral("previewGain"), previewGain);
        settings->setValue(QStringLiteral("mmcEnabled"), mmcEnabled.load());
        settings->endGroup();
    }

}
