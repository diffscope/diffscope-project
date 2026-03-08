#include "AudioOutputSettingsHelper.h"

#include <algorithm>
#include <QCoreApplication>
#include <QLocale>
#include <QSignalBlocker>

#include <TalcsCore/MixerAudioSource.h>
#include <TalcsDevice/AbstractOutputContext.h>
#include <TalcsDevice/AudioDevice.h>
#include <TalcsDevice/AudioDriver.h>
#include <TalcsDevice/AudioDriverManager.h>

#include <audio/internal/AudioSystem.h>
#include <audio/internal/OutputSystem.h>

namespace Audio::Internal {

    static OutputSystem *m_outputSystem;

    // DriverItem display name generation
    QString DriverItem::generateDisplayName() const {
        if (name.isEmpty()) {
            return QCoreApplication::translate("Audio::Internal::AudioOutputSettingsHelper", "(Not working)");
        }
        if (status == ItemStatus::NotWorking) {
            return name + " " + QCoreApplication::translate("Audio::Internal::AudioOutputSettingsHelper", "(Not working)");
        }
        return name;
    }

    // DeviceItem display name generation
    QString DeviceItem::generateDisplayName() const {
        QString baseName = name.isEmpty() 
            ? QCoreApplication::translate("Audio::Internal::AudioOutputSettingsHelper", "Default device")
            : name;
        
        if (status == ItemStatus::NotWorking) {
            return baseName + " " + QCoreApplication::translate("Audio::Internal::AudioOutputSettingsHelper", "(Not working)");
        }
        return baseName;
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

        // Initialize all lists and current indices
        updateDriverList();
        updateDeviceDependentLists();
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
            emit driverCurrentIndexChanged();
            return;
        }

        auto driverNames = outputContext->driverManager()->drivers();
        for (const QString &driverName : driverNames) {
            DriverItem item;
            item.name = driverName;
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

        if (m_driverCurrentIndex != newIndex) {
            m_driverCurrentIndex = newIndex;
            emit driverCurrentIndexChanged();
        }
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

        const auto &item = m_driverItems[index];

        // Return directly if selecting an empty name item (not working)
        if (item.name.isEmpty()) {
            return;
        }

        // Try to set driver
        if (!m_outputSystem->setDriver(item.name)) {
            // Setting failed, emit error signal
            emit deviceError(tr("Cannot initialize %1 driver").arg(item.name));

            // Mark current driver as NotWorking and add if not exists
            int existingIndex = findDriverIndexByName(item.name, ItemStatus::Normal);
            if (existingIndex != -1) {
                m_driverItems[existingIndex].status = ItemStatus::NotWorking;
            } else {
                DriverItem notWorkingItem;
                notWorkingItem.name = item.name;
                notWorkingItem.status = ItemStatus::NotWorking;
                m_driverItems.append(notWorkingItem);
            }

            // Find first empty name item to set as current
            for (int i = 0; i < m_driverItems.size(); ++i) {
                if (m_driverItems[i].name.isEmpty()) {
                    m_driverCurrentIndex = i;
                    emit driverCurrentIndexChanged();
                    break;
                }
            }
            return;
        }

        // Setting succeeded, remove NotWorking items
        removeNotWorkingDriverItems();

        // Adjust index after removal
        int newIndex = findDriverIndexByName(item.name, ItemStatus::Normal);
        if (newIndex != -1) {
            m_driverCurrentIndex = newIndex;
            emit driverCurrentIndexChanged();
        }

        // Driver change will automatically trigger deviceChanged signal, which updates device-related lists
    }

    // Update device list
    void AudioOutputSettingsHelper::updateDeviceList() {
        m_deviceItems.clear();

        auto outputContext = m_outputSystem->outputContext();
        if (!outputContext || !outputContext->driver()) {
            // Add "(Not working)" item when no driver is available
            DeviceItem item;
            item.name = QString();
            item.status = ItemStatus::Normal;
            m_deviceItems.append(item);
            m_deviceCurrentIndex = 0;
            emit deviceCurrentIndexChanged();
            emit deviceListChanged();
            return;
        }

        auto driver = outputContext->driver();

        // First, add default device from driver->defaultDevice() if exists
        QString driverDefaultDevice = driver->defaultDevice();
        if (!driverDefaultDevice.isEmpty()) {
            DeviceItem item;
            item.name = QString(); // Empty string indicates default device
            item.status = ItemStatus::Normal;
            m_deviceItems.append(item);
        }

        // Then add specific devices
        QStringList deviceNames = driver->devices();
        for (const QString &deviceName : deviceNames) {
            if (deviceName.isEmpty()) {
                // Handle devices with empty name (another type of default device)
                // Only add if we haven't already added a default device
                if (driverDefaultDevice.isEmpty()) {
                    DeviceItem item;
                    item.name = QString();
                    item.status = ItemStatus::Normal;
                    m_deviceItems.append(item);
                }
            } else {
                DeviceItem item;
                item.name = deviceName;
                item.status = ItemStatus::Normal;
                m_deviceItems.append(item);
            }
        }

        // Check current device status
        if (!outputContext->device()) {
            // No device instance, add "(Not working)" item if not already present
            if (m_deviceItems.isEmpty() || !m_deviceItems[0].name.isEmpty()) {
                DeviceItem item;
                item.name = QString();
                item.status = ItemStatus::Normal;
                m_deviceItems.append(item);
            }
        } else {
            // Check if device is in available list
            QString currentDeviceName = outputContext->device()->name();
            int foundIndex = findDeviceIndexByName(currentDeviceName, ItemStatus::Normal);

            if (foundIndex == -1) {
                // Device exists but not in available list, add as NotWorking
                DeviceItem item;
                item.name = currentDeviceName;
                item.status = ItemStatus::NotWorking;
                m_deviceItems.append(item);
            }
        }

        emit deviceListChanged();
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
            // If current device not found, find first empty name item
            for (int i = 0; i < m_deviceItems.size(); ++i) {
                if (m_deviceItems[i].name.isEmpty()) {
                    newIndex = i;
                    break;
                }
            }
            if (newIndex == -1) {
                newIndex = 0;
            }
        }

        if (m_deviceCurrentIndex != newIndex) {
            m_deviceCurrentIndex = newIndex;
            emit deviceCurrentIndexChanged();
        }
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

        const auto &item = m_deviceItems[index];

        // Return directly if selecting a NotWorking item
        if (item.status == ItemStatus::NotWorking) {
            emit deviceCurrentIndexChanged();
            return;
        }

        // Try to set device
        if (!m_outputSystem->setDevice(item.name)) {
            // Setting failed, emit error signal
            QString errorMessage = item.name.isEmpty() 
                ? tr("Audio device %1 is not available").arg(tr("Default device"))
                : tr("Audio device %1 is not available").arg(item.name);
            emit deviceError(errorMessage);

            // Keep current index unchanged
            emit deviceCurrentIndexChanged();
            return;
        }

        // Setting succeeded, remove any NotWorking items
        removeNotWorkingDeviceItems();

        // Adjust index after removal
        int newIndex = findDeviceIndexByName(item.name, ItemStatus::Normal);
        if (newIndex != -1) {
            m_deviceCurrentIndex = newIndex;
            emit deviceCurrentIndexChanged();
            emit deviceListChanged();
        }

        // Device change will automatically trigger deviceChanged signal, which updates sample rate and buffer size lists
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

        if (m_sampleRateCurrentIndex != newIndex) {
            m_sampleRateCurrentIndex = newIndex;
            emit sampleRateCurrentIndexChanged();
        }
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

        if (m_bufferSizeCurrentIndex != newIndex) {
            m_bufferSizeCurrentIndex = newIndex;
            emit bufferSizeCurrentIndexChanged();
        }
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

    // Bidirectional binding slot: respond to outputContext's deviceChanged signal
    void AudioOutputSettingsHelper::onDeviceChanged() {
        // Re-update all device-dependent lists when device changes
        updateDeviceDependentLists();
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
        auto outputContext = m_outputSystem->outputContext();
        if (!outputContext || !outputContext->controlMixer()) {
            return 1.0;
        }
        return static_cast<double>(outputContext->controlMixer()->gain());
    }

    void AudioOutputSettingsHelper::setDeviceGain(double gain) {
        auto outputContext = m_outputSystem->outputContext();
        if (!outputContext || !outputContext->controlMixer()) {
            return;
        }
        
        if (qFuzzyCompare(outputContext->controlMixer()->gain(), static_cast<float>(gain))) {
            return;
        }
        
        outputContext->controlMixer()->setGain(static_cast<float>(gain));
        emit deviceGainChanged();
    }

    double AudioOutputSettingsHelper::devicePan() const {
        auto outputContext = m_outputSystem->outputContext();
        if (!outputContext || !outputContext->controlMixer()) {
            return 0.0;
        }
        return static_cast<double>(outputContext->controlMixer()->pan());
    }

    void AudioOutputSettingsHelper::setDevicePan(double pan) {
        auto outputContext = m_outputSystem->outputContext();
        if (!outputContext || !outputContext->controlMixer()) {
            return;
        }
        
        if (qFuzzyCompare(outputContext->controlMixer()->pan(), static_cast<float>(pan))) {
            return;
        }
        
        outputContext->controlMixer()->setPan(static_cast<float>(pan));
        emit devicePanChanged();
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

    // Helper method: Remove NotWorking driver items
    void AudioOutputSettingsHelper::removeNotWorkingDriverItems() {
        m_driverItems.erase(
            std::remove_if(m_driverItems.begin(), m_driverItems.end(),
                [](const DriverItem &item) {
                    return item.status == ItemStatus::NotWorking;
                }),
            m_driverItems.end()
        );
    }

    // Helper method: Remove NotWorking device items
    void AudioOutputSettingsHelper::removeNotWorkingDeviceItems() {
        m_deviceItems.erase(
            std::remove_if(m_deviceItems.begin(), m_deviceItems.end(),
                [](const DeviceItem &item) {
                    return item.status == ItemStatus::NotWorking;
                }),
            m_deviceItems.end()
        );
    }

}
