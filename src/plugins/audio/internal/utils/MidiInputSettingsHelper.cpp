// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "MidiInputSettingsHelper.h"

#include <QRandomGenerator>
#include <QTimer>
#include <QVariantMap>

#include <audio/MidiInputContext.h>
#include <audio/internal/WaveformTypeCatalog.h>

namespace Audio::Internal {

    bool MidiInputSettingsHelper::m_easterEggEnabled{};

    MidiInputSettingsHelper::MidiInputSettingsHelper(QObject *parent)
        : QObject(parent), m_context(MidiInputContext::instance()) {
        connect(m_context.data(), &MidiInputContext::devicesChanged, this, &MidiInputSettingsHelper::updateDeviceList);
        connect(m_context.data(), &MidiInputContext::deviceChanged, this, &MidiInputSettingsHelper::updateDeviceList);
        connect(m_context.data(), &MidiInputContext::previewEnabledChanged, this, &MidiInputSettingsHelper::previewEnabledChanged);
        connect(m_context.data(), &MidiInputContext::previewTypeChanged, this, &MidiInputSettingsHelper::previewTypeCurrentIndexChanged);
        connect(m_context.data(), &MidiInputContext::previewGainChanged, this, &MidiInputSettingsHelper::previewGainChanged);
        connect(m_context.data(), &MidiInputContext::mmcEnabledChanged, this, &MidiInputSettingsHelper::mmcEnabledChanged);
        m_previewTestTimer = new QTimer(this);
        m_previewTestTimer->setInterval(20);
        connect(m_previewTestTimer, &QTimer::timeout, this, [this] {
            if (!previewTestPlaying())
                finishPreviewTest();
        });
        m_context->refreshDevices();
        updateDeviceList();
    }

    MidiInputSettingsHelper::~MidiInputSettingsHelper() {
        finishPreviewTest();
    }

    QStringList MidiInputSettingsHelper::deviceList() const {
        return m_deviceList;
    }

    int MidiInputSettingsHelper::deviceCurrentIndex() const {
        return m_deviceCurrentIndex;
    }

    void MidiInputSettingsHelper::setDeviceCurrentIndex(int index) {
        if (index < 0 || index >= m_deviceIds.size() || index == m_deviceCurrentIndex)
            return;
        const auto id = m_deviceIds.at(index);
        const auto name = m_deviceList.at(index);
        const bool succeeded = m_context->setDeviceId(id);
        updateDeviceList();
        if (!succeeded) {
            const auto error = m_context->deviceErrorString();
            Q_EMIT deviceError(error.isEmpty() ? tr("MIDI device %1 is not available.").arg(name)
                                             : tr("Cannot open MIDI device %1.\n\n%2").arg(name, error));
        }
    }

    bool MidiInputSettingsHelper::previewEnabled() const {
        return m_context->previewEnabled();
    }

    void MidiInputSettingsHelper::setPreviewEnabled(bool enabled) {
        m_context->setPreviewEnabled(enabled);
    }

    QStringList MidiInputSettingsHelper::previewTypeList() const {
        QStringList names;
        for (const auto &entry : WaveformTypeCatalog::availableTypes())
            names.append(entry.name);
        return names;
    }

    int MidiInputSettingsHelper::previewTypeCurrentIndex() const {
        return WaveformTypeCatalog::indexOf(m_context->previewType());
    }

    void MidiInputSettingsHelper::setPreviewTypeCurrentIndex(int index) {
        const auto &types = WaveformTypeCatalog::availableTypes();
        if (index >= 0 && index < types.size())
            m_context->setPreviewType(types.at(index).type);
    }

    double MidiInputSettingsHelper::previewGain() const {
        return m_context->previewGain();
    }

    void MidiInputSettingsHelper::setPreviewGain(double gain) {
        m_context->setPreviewGain(gain);
    }

    bool MidiInputSettingsHelper::mmcEnabled() const {
        return m_context->mmcEnabled();
    }

    void MidiInputSettingsHelper::setMmcEnabled(bool enabled) {
        m_context->setMmcEnabled(enabled);
    }

    bool MidiInputSettingsHelper::previewTestPlaying() const {
        return m_previewTestPlaying && m_context && m_context->isPreviewTestPlaying();
    }

    void MidiInputSettingsHelper::setPreviewTestPlaying(bool playing) {
        if (playing) {
            if (previewTestPlaying())
                return;
            finishPreviewTest();
            if (!m_context->playPreviewTest(m_easterEggEnabled && QRandomGenerator::global()->bounded(2) == 0)) {
                Q_EMIT previewTestPlayingChanged();
                return;
            }
            m_previewTestPlaying = true;
            m_previewTestTimer->start();
            Q_EMIT previewTestPlayingChanged();
        } else {
            finishPreviewTest();
        }
    }

    QString MidiInputSettingsHelper::previewTestToolTip() const {
        return m_previewTestToolTip;
    }

    int MidiInputSettingsHelper::previewTestInputLength() const {
        return m_easterEggInputLength;
    }

    void MidiInputSettingsHelper::interruptPreview() {
        m_context->interruptPreview();
        finishPreviewTest();
    }

    void MidiInputSettingsHelper::inputPreviewTestText(const QString &text) {
        if (!previewTestPlaying())
            return;
        auto length = m_easterEggInputLength;
        for (const auto character : text) {
            if (character == QLatin1Char('!')) {
                length = length == 5 ? 0 : length + 1;
            } else {
                length = 0;
            }
        }
        if (text.isEmpty())
            length = 0;
        if (length != m_easterEggInputLength) {
            m_easterEggInputLength = length;
            Q_EMIT previewTestInputLengthChanged();
        }
    }

    void MidiInputSettingsHelper::finishPreviewTestInput() {
        if (m_easterEggInputLength != 0 && m_easterEggInputLength != 5) {
            m_easterEggInputLength = 0;
            Q_EMIT previewTestInputLengthChanged();
        }
    }

    void MidiInputSettingsHelper::finishPreviewTest() {
        if (!m_previewTestPlaying)
            return;
        m_previewTestTimer->stop();
        if (m_context)
            m_context->stopPreviewTest();
        if (m_easterEggInputLength == 5)
            m_easterEggEnabled = !m_easterEggEnabled;
        m_easterEggInputLength = 0;
        m_previewTestPlaying = false;
        m_previewTestToolTip = m_easterEggEnabled ? (QRandomGenerator::global()->bounded(2) ? tr("Why did you play ...?!") : tr("I'll put an end to it.")) : QString();
        Q_EMIT previewTestToolTipChanged();
        Q_EMIT previewTestInputLengthChanged();
        Q_EMIT previewTestPlayingChanged();
    }

    void MidiInputSettingsHelper::updateDeviceList() {
        m_deviceIds = {QString()};
        m_deviceList = {tr("(None)")};
        const auto currentId = m_context->deviceId();
        const bool ready = m_context->isDeviceReady();
        for (const auto &entry : m_context->devices()) {
            const auto device = entry.toMap();
            const auto id = device.value(QStringLiteral("value")).toString();
            auto name = device.value(QStringLiteral("name")).toString();
            if (id == currentId && !ready)
                name = tr("%1 (Not working)").arg(name);
            m_deviceIds.append(id);
            m_deviceList.append(name);
        }
        m_deviceCurrentIndex = static_cast<int>(m_deviceIds.indexOf(currentId));
        if (m_deviceCurrentIndex < 0) {
            m_deviceCurrentIndex = static_cast<int>(m_deviceIds.size());
            m_deviceIds.append(currentId);
            m_deviceList.append(tr("%1 (Not working)").arg(m_context->deviceName()));
        }
        Q_EMIT deviceListChanged();
        Q_EMIT deviceCurrentIndexChanged();
    }

}
