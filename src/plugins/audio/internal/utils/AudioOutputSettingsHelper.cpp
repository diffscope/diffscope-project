
#include "AudioOutputSettingsHelper.h"

#include <QCoreApplication>
#include <QLocale>
#include <QSignalBlocker>

#include <TalcsDevice/AbstractOutputContext.h>
#include <TalcsDevice/AudioDevice.h>
#include <TalcsDevice/AudioDriver.h>
#include <TalcsDevice/AudioDriverManager.h>

#include <audio/internal/audiosystem.h>
#include <audio/internal/outputsystem.h>

namespace Audio::Internal {

    static OutputSystem *m_outputSystem;

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
        m_driverList.clear();

        auto outputContext = m_outputSystem->outputContext();
        if (!outputContext || !outputContext->driverManager()) {
            // Add "(Not working)" option when no driver manager is available
            m_driverList.append(tr("(Not working)"));
            m_driverCurrentIndex = 0;
            emit driverCurrentIndexChanged();
            return;
        }

        auto driverNames = outputContext->driverManager()->drivers();
        for (const QString &driverName : driverNames) {
            m_driverList.append(driverName); // Use driver name directly without display name conversion
        }

        // Check current driver status
        if (!outputContext->driver()) {
            // Add "(Not working)" option when no driver is available
            m_driverList.append(tr("(Not working)"));
        }

        findDriverIndex();
    }

    // Find the index of current driver
    void AudioOutputSettingsHelper::findDriverIndex() {
        int newIndex = -1;
        auto outputContext = m_outputSystem->outputContext();

        if (outputContext && outputContext->driver()) {
            QString currentDriverName = outputContext->driver()->name();
            newIndex = m_driverList.indexOf(currentDriverName);
        }

        if (newIndex == -1) {
            // If current driver not found, check for "(Not working)" option
            int notWorkingIndex = m_driverList.indexOf(tr("(Not working)"));
            newIndex = notWorkingIndex != -1 ? notWorkingIndex : 0;
        }

        if (m_driverCurrentIndex != newIndex) {
            m_driverCurrentIndex = newIndex;
            emit driverCurrentIndexChanged();
        }
    }

    QStringList AudioOutputSettingsHelper::driverList() const {
        return m_driverList;
    }

    int AudioOutputSettingsHelper::driverCurrentIndex() const {
        return m_driverCurrentIndex;
    }

    void AudioOutputSettingsHelper::setDriverCurrentIndex(int index) {
        if (m_driverCurrentIndex == index || index < 0 || index >= m_driverList.size()) {
            return;
        }

        QString driverName = m_driverList[index];

        // Return directly if "(Not working)" option is selected
        if (driverName == tr("(Not working)")) {
            return;
        }

        // Try to set driver
        if (!m_outputSystem->setDriver(driverName)) {
            // Setting failed, emit error signal
            emit deviceError(tr("Cannot initialize %1 driver").arg(driverName));

            // Ensure "(Not working)" option exists in list
            if (!m_driverList.contains(tr("(Not working)"))) {
                m_driverList.append(tr("(Not working)"));
            }

            // Set to "(Not working)" option
            int notWorkingIndex = m_driverList.indexOf(tr("(Not working)"));
            m_driverCurrentIndex = notWorkingIndex;
            emit driverCurrentIndexChanged();
            return;
        }

        // Setting succeeded, remove "(Not working)" option if exists
        int notWorkingIndex = m_driverList.indexOf(tr("(Not working)"));
        if (notWorkingIndex != -1) {
            m_driverList.removeAt(notWorkingIndex);
            // Adjust index if removed option is before current option
            if (notWorkingIndex < index) {
                index--;
            }
        }

        m_driverCurrentIndex = index;
        emit driverCurrentIndexChanged();

        // Driver change will automatically trigger deviceChanged signal, which updates device-related lists
    }

    // Update device list
    void AudioOutputSettingsHelper::updateDeviceList() {
        m_deviceList.clear();
        m_actualDeviceList.clear();

        auto outputContext = m_outputSystem->outputContext();
        if (!outputContext || !outputContext->driver()) {
            // Add "(Not working)" option when no driver is available
            m_deviceList.append(tr("(Not working)"));
            m_actualDeviceList.append(QString()); // Empty actual name for not working
            m_deviceCurrentIndex = 0;
            emit deviceCurrentIndexChanged();
            emit deviceListChanged();
            return;
        }

        auto driver = outputContext->driver();

        // First, add default device from driver->defaultDevice() if exists
        QString driverDefaultDevice = driver->defaultDevice();
        if (!driverDefaultDevice.isEmpty()) {
            m_deviceList.append(tr("Default device"));
            m_actualDeviceList.append(QString()); // Empty string indicates default device
        }

        // Then add specific devices
        QStringList deviceNames = driver->devices();
        for (const QString &deviceName : deviceNames) {
            if (deviceName.isEmpty()) {
                // Handle devices with empty name (another type of default device)
                // Only add if we haven't already added a default device
                if (driverDefaultDevice.isEmpty()) {
                    m_deviceList.append(tr("Default device"));
                    m_actualDeviceList.append(QString()); // Empty string indicates default device
                }
            } else {
                m_deviceList.append(deviceName);
                m_actualDeviceList.append(deviceName);
            }
        }

        // Check current device status
        if (!outputContext->device()) {
            // No device instance, add "(Not working)" option
            m_deviceList.append(tr("(Not working)"));
            m_actualDeviceList.append(QString()); // Empty actual name for not working
        } else {
            // Check if device is in available list
            bool currentDeviceFound = false;
            QString currentDeviceName = outputContext->device()->name();

            if (currentDeviceName.isEmpty()) {
                // Current device has empty name (default device)
                // Check if we have a default device option
                int defaultIndex = m_deviceList.indexOf(tr("Default device"));
                if (defaultIndex != -1) {
                    currentDeviceFound = true;
                }
            } else if (m_actualDeviceList.contains(currentDeviceName)) {
                // Current device is in available list
                currentDeviceFound = true;
            }

            if (!currentDeviceFound) {
                // Device exists but not in available list, add option with "(Not working)" suffix
                QString displayName = currentDeviceName.isEmpty() ? tr("Default device") + " " + tr("(Not working)") : currentDeviceName + " " + tr("(Not working)");
                m_deviceList.append(displayName);
                m_actualDeviceList.append(currentDeviceName);
            }
        }

        findDeviceIndex();
        emit deviceListChanged();
    }

    // Find the index of current device
    void AudioOutputSettingsHelper::findDeviceIndex() {
        int newIndex = -1;
        auto outputContext = m_outputSystem->outputContext();

        if (outputContext && outputContext->device()) {
            QString currentDeviceName = outputContext->device()->name();

            if (currentDeviceName.isEmpty()) {
                // Current device has empty name (default device)
                // Find the first "Default device" option
                int defaultIndex = m_deviceList.indexOf(tr("Default device"));
                if (defaultIndex != -1) {
                    newIndex = defaultIndex;
                }
            } else {
                // Find specific device by actual device name
                newIndex = m_actualDeviceList.indexOf(currentDeviceName);
            }

            // If not found in normal list, check for "(Not working)" variants
            if (newIndex == -1) {
                QString notWorkingDisplayName = currentDeviceName.isEmpty() ? tr("Default device") + " " + tr("(Not working)") : currentDeviceName + " " + tr("(Not working)");
                newIndex = m_deviceList.indexOf(notWorkingDisplayName);
            }
        }

        if (newIndex == -1) {
            // If current device not found, select "(Not working)" option
            int notWorkingIndex = m_deviceList.indexOf(tr("(Not working)"));
            newIndex = notWorkingIndex != -1 ? notWorkingIndex : 0;
        }

        if (m_deviceCurrentIndex != newIndex) {
            m_deviceCurrentIndex = newIndex;
            emit deviceCurrentIndexChanged();
        }
    }

    // Get actual device name based on index
    QString AudioOutputSettingsHelper::getActualDeviceName(int index) const {
        if (index < 0 || index >= m_actualDeviceList.size()) {
            return QString();
        }

        return m_actualDeviceList[index];
    }

    QStringList AudioOutputSettingsHelper::deviceList() const {
        return m_deviceList;
    }

    int AudioOutputSettingsHelper::deviceCurrentIndex() const {
        return m_deviceCurrentIndex;
    }

    void AudioOutputSettingsHelper::setDeviceCurrentIndex(int index) {
        if (m_deviceCurrentIndex == index || index < 0 || index >= m_deviceList.size()) {
            return;
        }

        QString deviceName = getActualDeviceName(index);
        QString displayName = m_deviceList[index];

        // Return directly if "(Not working)" option is selected
        if (displayName == tr("(Not working)") || displayName.contains(tr("(Not working)"))) {
            return;
        }

        // Try to set device
        if (!m_outputSystem->setDevice(deviceName)) {
            // Setting failed, emit error signal
            QString errorMessage = deviceName.isEmpty() ? tr("Audio device %1 is not available").arg(tr("Default device")) : tr("Audio device %1 is not available").arg(deviceName);
            emit deviceError(errorMessage);

            // Keep current index unchanged
            return;
        }

        // Setting succeeded, remove any "(Not working)" options
        for (int i = m_deviceList.size() - 1; i >= 0; i--) {
            if (m_deviceList[i] == tr("(Not working)") ||
                m_deviceList[i].contains(tr("(Not working)"))) {
                m_deviceList.removeAt(i);
                m_actualDeviceList.removeAt(i);
                if (i < index) {
                    index--; // Adjust index
                }
            }
        }

        m_deviceCurrentIndex = index;
        emit deviceCurrentIndexChanged();
        emit deviceListChanged();

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

        findSampleRateIndex();
        emit sampleRateListChanged();
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

        findBufferSizeIndex();
        emit bufferSizeListChanged();
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

}
