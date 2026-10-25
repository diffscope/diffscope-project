// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "AudioOutputSettingsHelper.h"

#include <QCoreApplication>
#include <QLocale>

#include <TalcsDevice/AbstractOutputContext.h>
#include <TalcsDevice/AudioDevice.h>
#include <TalcsDevice/AudioDriver.h>
#include <TalcsDevice/AudioDriverManager.h>

#include <audio/GlobalAudioContext.h>
#include <audio/internal/AudioSystem.h>
#include <audio/internal/OutputSystem.h>

namespace Audio::Internal {

    static OutputSystem *m_outputSystem;

    // DriverItem display name generation
    QString DriverItem::generateDisplayName() const {
        if (name.isEmpty()) {
            return QCoreApplication::translate("Audio::Internal::AudioOutputSettingsHelper", "(Not working)");
        }
        const auto text = displayName.isEmpty() ? name : displayName;
        if (status == ItemStatus::NotWorking) {
            return text + " " + QCoreApplication::translate("Audio::Internal::AudioOutputSettingsHelper", "(Not working)");
        }
        return text;
    }

    // DeviceItem display name generation
    QString DeviceItem::generateDisplayName() const {
        if (name.isEmpty()) {
            return QCoreApplication::translate("Audio::Internal::AudioOutputSettingsHelper", "(Not working)");
        }
        if (status == ItemStatus::NotWorking) {
            return name + " " + QCoreApplication::translate("Audio::Internal::AudioOutputSettingsHelper", "(Not working)");
        }
        return name;
    }

    AudioOutputSettingsHelper::AudioOutputSettingsHelper(QObject *parent)
        : QObject(parent), m_driverCurrentIndex(-1), m_deviceCurrentIndex(-1),
          m_sampleRateCurrentIndex(-1), m_bufferSizeCurrentIndex(-1) {

        if (!m_outputSystem) {
            m_outputSystem = AudioSystem::outputSystem();
        }

        // Connect outputContext signals for bidirectional binding
        auto outputContext = m_outputSystem->outputContext();
        connect(outputContext, &talcs::AbstractOutputContext::deviceChanged, this, &AudioOutputSettingsHelper::onDeviceChanged);
        connect(outputContext, &talcs::AbstractOutputContext::bufferSizeChanged, this, &AudioOutputSettingsHelper::onBufferSizeChanged);
        connect(outputContext, &talcs::AbstractOutputContext::sampleRateChanged, this, &AudioOutputSettingsHelper::onSampleRateChanged);
        connect(GlobalAudioContext::instance(), &GlobalAudioContext::deviceGainChanged,
                this, [this] { Q_EMIT deviceGainChanged(); });
        connect(GlobalAudioContext::instance(), &GlobalAudioContext::devicePanChanged,
                this, [this] { Q_EMIT devicePanChanged(); });

        reload();
    }

    AudioOutputSettingsHelper::~AudioOutputSettingsHelper() = default;

    // Update driver list
    void AudioOutputSettingsHelper::updateDriverList() {
        m_driverItems.clear();

        auto outputContext = m_outputSystem->outputContext();
        if (!outputContext || !outputContext->driverManager()) {
            // Add "(Not working)" item when no driver manager is available
            DriverItem item;
            item.name = QString();
            item.status = ItemStatus::Normal;
            m_driverItems.append(item);
            m_driverCurrentIndex = 0;
            Q_EMIT driverListChanged();
            Q_EMIT driverCurrentIndexChanged();
            return;
        }

        auto driverNames = outputContext->driverManager()->drivers();
        for (const QString &driverName : driverNames) {
            DriverItem item;
            item.name = driverName;
            item.displayName = outputContext->driverManager()->driver(driverName)->displayName();
            item.status = ItemStatus::Normal;
            m_driverItems.append(item);
        }

        // Check current driver status
        if (!outputContext->driver()) {
            // Add "(Not working)" item when no driver is available
            DriverItem item;
            item.name = QString();
            item.status = ItemStatus::Normal;
            m_driverItems.append(item);
        }

        Q_EMIT driverListChanged();
        findDriverIndex();
    }

    // Find the index of current driver
    void AudioOutputSettingsHelper::findDriverIndex() {
        int newIndex = -1;
        auto outputContext = m_outputSystem->outputContext();

        if (outputContext && outputContext->driver()) {
            QString currentDriverName = outputContext->driver()->name();
            newIndex = findDriverIndexByName(currentDriverName, ItemStatus::Normal);
        }

        if (newIndex == -1) {
            // If current driver not found, find first empty name item (not working)
            for (int i = 0; i < m_driverItems.size(); ++i) {
                if (m_driverItems[i].name.isEmpty()) {
                    newIndex = i;
                    break;
                }
            }
            if (newIndex == -1) {
                newIndex = 0;
            }
        }

        m_driverCurrentIndex = newIndex;
        Q_EMIT driverCurrentIndexChanged();
    }

    QStringList AudioOutputSettingsHelper::driverList() const {
        QStringList list;
        for (const auto &item : m_driverItems) {
            list.append(item.generateDisplayName());
        }
        return list;
    }

    int AudioOutputSettingsHelper::driverCurrentIndex() const {
        return m_driverCurrentIndex;
    }

    void AudioOutputSettingsHelper::setDriverCurrentIndex(int index) {
        if (m_driverCurrentIndex == index || index < 0 || index >= m_driverItems.size()) {
            return;
        }

        const auto item = m_driverItems[index];

        // Return directly if selecting an empty name item (not working)
        if (item.name.isEmpty()) {
            Q_EMIT driverCurrentIndexChanged();
            return;
        }

        const bool succeeded = m_outputSystem->setDriver(item.name);
        reload();
        if (!succeeded) {
            Q_EMIT deviceError(tr("Cannot initialize %1 driver").arg(item.generateDisplayName()));
        }
    }

    // Update device list
    void AudioOutputSettingsHelper::updateDeviceList() {
        m_deviceItems.clear();

        auto outputContext = m_outputSystem->outputContext();
        if (outputContext && outputContext->driver()) {
            const auto deviceNames = outputContext->driver()->devices();
            for (const auto &deviceName : deviceNames) {
                if (!deviceName.isEmpty()) {
                    m_deviceItems.append({deviceName, ItemStatus::Normal});
                }
            }
        }

        auto *device = outputContext ? outputContext->device() : nullptr;
        if (!device) {
            m_deviceItems.append({QString(), ItemStatus::NotWorking});
        } else {
            const auto currentDeviceName = device->name();
            if (findDeviceIndexByName(currentDeviceName, ItemStatus::Normal) == -1) {
                m_deviceItems.append({currentDeviceName, ItemStatus::NotWorking});
            }
        }

        Q_EMIT deviceListChanged();
        findDeviceIndex();
    }

    // Find the index of current device
    void AudioOutputSettingsHelper::findDeviceIndex() {
        int newIndex = -1;
        auto outputContext = m_outputSystem->outputContext();

        if (outputContext && outputContext->device()) {
            QString currentDeviceName = outputContext->device()->name();
            
            // First try to find in normal items
            newIndex = findDeviceIndexByName(currentDeviceName, ItemStatus::Normal);
            
            // If not found, try to find in NotWorking items
            if (newIndex == -1) {
                newIndex = findDeviceIndexByName(currentDeviceName, ItemStatus::NotWorking);
            }
        }

        if (newIndex == -1) {
            newIndex = findDeviceIndexByName(QString(), ItemStatus::NotWorking);
        }
        if (newIndex == -1 && !m_deviceItems.isEmpty()) {
            newIndex = 0;
        }

        m_deviceCurrentIndex = newIndex;
        Q_EMIT deviceCurrentIndexChanged();
    }

    QStringList AudioOutputSettingsHelper::deviceList() const {
        QStringList list;
        for (const auto &item : m_deviceItems) {
            list.append(item.generateDisplayName());
        }
        return list;
    }

    int AudioOutputSettingsHelper::deviceCurrentIndex() const {
        return m_deviceCurrentIndex;
    }

    void AudioOutputSettingsHelper::setDeviceCurrentIndex(int index) {
        if (m_deviceCurrentIndex == index || index < 0 || index >= m_deviceItems.size()) {
            return;
        }

        const auto item = m_deviceItems[index];

        // Return directly if selecting a NotWorking item
        if (item.status == ItemStatus::NotWorking || item.name.isEmpty()) {
            Q_EMIT deviceCurrentIndexChanged();
            return;
        }

        const bool succeeded = m_outputSystem->setDevice(item.name);
        reload();
        if (!succeeded) {
            Q_EMIT deviceError(tr("Audio device %1 is not available").arg(item.name));
        }
    }

    // Update sample rate list
    void AudioOutputSettingsHelper::updateSampleRateList() {
        m_sampleRateList.clear();

        auto outputContext = m_outputSystem->outputContext();
        if (!outputContext || !outputContext->device()) {
            m_sampleRateCurrentIndex = -1;
            emit sampleRateCurrentIndexChanged();
            emit sampleRateListChanged();
            return;
        }

        auto device = outputContext->device();
        auto sampleRateList = device->availableSampleRates();

        QLocale locale; // Use current system locale
        for (double rate : sampleRateList) {
            m_sampleRateList.append(locale.toString(rate));
        }

        emit sampleRateListChanged();
        findSampleRateIndex();
    }

    // Find current sample rate index
    void AudioOutputSettingsHelper::findSampleRateIndex() {
        int newIndex = -1;
        auto outputContext = m_outputSystem->outputContext();

        if (outputContext && outputContext->device()) {
            double currentSampleRate = outputContext->adoptedSampleRate();
            auto device = outputContext->device();
            auto sampleRateList = device->availableSampleRates();

            QLocale locale;
            for (int i = 0; i < sampleRateList.size(); i++) {
                if (qFuzzyCompare(sampleRateList[i], currentSampleRate)) {
                    newIndex = i;
                    break;
                }
            }
        }

        m_sampleRateCurrentIndex = newIndex;
        Q_EMIT sampleRateCurrentIndexChanged();
    }

    // Update buffer size list
    void AudioOutputSettingsHelper::updateBufferSizeList() {
        m_bufferSizeList.clear();

        auto outputContext = m_outputSystem->outputContext();
        if (!outputContext || !outputContext->device()) {
            m_bufferSizeCurrentIndex = -1;
            emit bufferSizeCurrentIndexChanged();
            emit bufferSizeListChanged();
            return;
        }

        auto device = outputContext->device();
        auto bufferSizeList = device->availableBufferSizes();

        QLocale locale; // Use current system locale
        for (qint64 size : bufferSizeList) {
            m_bufferSizeList.append(locale.toString(size));
        }

        emit bufferSizeListChanged();
        findBufferSizeIndex();
    }

    // Find current buffer size index
    void AudioOutputSettingsHelper::findBufferSizeIndex() {
        int newIndex = -1;
        auto outputContext = m_outputSystem->outputContext();

        if (outputContext && outputContext->device()) {
            qint64 currentBufferSize = outputContext->adoptedBufferSize();
            auto device = outputContext->device();
            auto bufferSizeList = device->availableBufferSizes();

            for (int i = 0; i < bufferSizeList.size(); i++) {
                if (bufferSizeList[i] == currentBufferSize) {
                    newIndex = i;
                    break;
                }
            }
        }

        m_bufferSizeCurrentIndex = newIndex;
        Q_EMIT bufferSizeCurrentIndexChanged();
    }

    // Update all lists dependent on device
    void AudioOutputSettingsHelper::updateDeviceDependentLists() {
        updateDeviceList();
        updateSampleRateList();
        updateBufferSizeList();
    }

    QStringList AudioOutputSettingsHelper::sampleRateList() const {
        return m_sampleRateList;
    }

    int AudioOutputSettingsHelper::sampleRateCurrentIndex() const {
        return m_sampleRateCurrentIndex;
    }

    QStringList AudioOutputSettingsHelper::bufferSizeList() const {
        return m_bufferSizeList;
    }

    int AudioOutputSettingsHelper::bufferSizeCurrentIndex() const {
        return m_bufferSizeCurrentIndex;
    }

    void AudioOutputSettingsHelper::setSampleRateCurrentIndex(int index) {
        if (m_sampleRateCurrentIndex == index || index < 0 || index >= m_sampleRateList.size()) {
            return;
        }

        auto outputContext = m_outputSystem->outputContext();
        if (!outputContext || !outputContext->device()) {
            return;
        }

        auto device = outputContext->device();
        auto sampleRateList = device->availableSampleRates();

        if (index >= sampleRateList.size()) {
            return;
        }

        double newSampleRate = sampleRateList[index];

        // Try to set sample rate
        if (!m_outputSystem->setAdoptedSampleRate(newSampleRate)) {
            // Setting failed, emit error signal
            QLocale locale;
            emit deviceError(tr("Cannot set sample rate to %1").arg(locale.toString(newSampleRate)));
            return;
        }

        m_sampleRateCurrentIndex = index;
        emit sampleRateCurrentIndexChanged();
    }

    void AudioOutputSettingsHelper::setBufferSizeCurrentIndex(int index) {
        if (m_bufferSizeCurrentIndex == index || index < 0 || index >= m_bufferSizeList.size()) {
            return;
        }

        auto outputContext = m_outputSystem->outputContext();
        if (!outputContext || !outputContext->device()) {
            return;
        }

        auto device = outputContext->device();
        auto bufferSizeList = device->availableBufferSizes();

        if (index >= bufferSizeList.size()) {
            return;
        }

        qint64 newBufferSize = bufferSizeList[index];

        // Try to set buffer size
        if (!m_outputSystem->setAdoptedBufferSize(newBufferSize)) {
            // Setting failed, emit error signal
            QLocale locale;
            emit deviceError(tr("Cannot set buffer size to %1").arg(locale.toString(newBufferSize)));
            return;
        }

        m_bufferSizeCurrentIndex = index;
        emit bufferSizeCurrentIndexChanged();
    }

    void AudioOutputSettingsHelper::testDevice() {
        m_outputSystem->playTestSound();
    }

    void AudioOutputSettingsHelper::openControlPanel() {
        auto dev = m_outputSystem->outputContext()->device();
        if (!dev)
            return;
        dev->openControlPanel();
    }

    void AudioOutputSettingsHelper::reload() {
        // List replacement requires index notifications even if the selected indices are unchanged.
        updateDriverList();
        updateDeviceDependentLists();
    }

    void AudioOutputSettingsHelper::onDeviceChanged() {
        reload();
    }

    // Bidirectional binding slot: respond to outputContext's bufferSizeChanged signal
    void AudioOutputSettingsHelper::onBufferSizeChanged(qint64 bufferSize) {
        Q_UNUSED(bufferSize) // Don't use parameter, directly re-find index
        findBufferSizeIndex();
    }

    // Bidirectional binding slot: respond to outputContext's sampleRateChanged signal
    void AudioOutputSettingsHelper::onSampleRateChanged(double sampleRate) {
        Q_UNUSED(sampleRate) // Don't use parameter, directly re-find index
        findSampleRateIndex();
    }

    double AudioOutputSettingsHelper::deviceGain() const {
        return GlobalAudioContext::deviceGain();
    }

    void AudioOutputSettingsHelper::setDeviceGain(double gain) {
        GlobalAudioContext::setDeviceGain(gain);
    }

    double AudioOutputSettingsHelper::devicePan() const {
        return GlobalAudioContext::devicePan();
    }

    void AudioOutputSettingsHelper::setDevicePan(double pan) {
        GlobalAudioContext::setDevicePan(pan);
    }

    // Helper method: Find driver index by name
    int AudioOutputSettingsHelper::findDriverIndexByName(const QString &driverName, ItemStatus status) const {
        for (int i = 0; i < m_driverItems.size(); ++i) {
            if (m_driverItems[i].name == driverName && m_driverItems[i].status == status) {
                return i;
            }
        }
        return -1;
    }

    // Helper method: Find device index by name
    int AudioOutputSettingsHelper::findDeviceIndexByName(const QString &deviceName, ItemStatus status) const {
        for (int i = 0; i < m_deviceItems.size(); ++i) {
            if (m_deviceItems[i].name == deviceName && m_deviceItems[i].status == status) {
                return i;
            }
        }
        return -1;
    }

}
