#ifndef DIFFSCOPE_AUDIO_AUDIOOUTPUTSETTINGSHELPER_H
#define DIFFSCOPE_AUDIO_AUDIOOUTPUTSETTINGSHELPER_H

#include <qqmlintegration.h>

#include <QObject>
#include <QStringList>

namespace Audio::Internal {

    class AudioOutputSettingsHelper : public QObject {
        Q_OBJECT
        QML_ELEMENT

        Q_PROPERTY(QStringList driverList READ driverList CONSTANT)
        Q_PROPERTY(int driverCurrentIndex READ driverCurrentIndex WRITE setDriverCurrentIndex NOTIFY driverCurrentIndexChanged)
        Q_PROPERTY(QStringList deviceList READ deviceList NOTIFY deviceListChanged)
        Q_PROPERTY(int deviceCurrentIndex READ deviceCurrentIndex WRITE setDeviceCurrentIndex NOTIFY deviceCurrentIndexChanged)
        Q_PROPERTY(QStringList sampleRateList READ sampleRateList NOTIFY sampleRateListChanged)
        Q_PROPERTY(int sampleRateCurrentIndex READ sampleRateCurrentIndex WRITE setSampleRateCurrentIndex NOTIFY sampleRateCurrentIndexChanged)
        Q_PROPERTY(QStringList bufferSizeList READ bufferSizeList NOTIFY bufferSizeListChanged)
        Q_PROPERTY(int bufferSizeCurrentIndex READ bufferSizeCurrentIndex WRITE setBufferSizeCurrentIndex NOTIFY bufferSizeCurrentIndexChanged)

    public:
        explicit AudioOutputSettingsHelper(QObject *parent = nullptr);
        ~AudioOutputSettingsHelper() override;

        QStringList driverList() const;

        int driverCurrentIndex() const;
        void setDriverCurrentIndex(int index);

        QStringList deviceList() const;

        int deviceCurrentIndex() const;
        void setDeviceCurrentIndex(int index);

        QStringList sampleRateList() const;

        int sampleRateCurrentIndex() const;
        void setSampleRateCurrentIndex(int index);

        QStringList bufferSizeList() const;

        int bufferSizeCurrentIndex() const;
        void setBufferSizeCurrentIndex(int index);

        Q_INVOKABLE void testDevice();
        Q_INVOKABLE static void openControlPanel();

    Q_SIGNALS:
        void driverCurrentIndexChanged();
        void deviceListChanged();
        void deviceCurrentIndexChanged();
        void sampleRateListChanged();
        void sampleRateCurrentIndexChanged();
        void bufferSizeListChanged();
        void bufferSizeCurrentIndexChanged();
        void deviceError(const QString &message);

    private Q_SLOTS:
        void onDeviceChanged();
        void onBufferSizeChanged(qint64 bufferSize);
        void onSampleRateChanged(double sampleRate);

    private:
        void updateDriverList();
        void updateDeviceDependentLists();
        void updateDeviceList();
        void updateSampleRateList();
        void updateBufferSizeList();

        void findDriverIndex();
        void findDeviceIndex();
        void findSampleRateIndex();
        void findBufferSizeIndex();

        QString getActualDeviceName(int index) const;

        QStringList m_driverList;
        int m_driverCurrentIndex;
        QStringList m_deviceList;
        QStringList m_actualDeviceList; // Store actual device names corresponding to display names
        int m_deviceCurrentIndex;
        QStringList m_sampleRateList;
        int m_sampleRateCurrentIndex;
        QStringList m_bufferSizeList;
        int m_bufferSizeCurrentIndex;
    };

}

#endif //DIFFSCOPE_AUDIO_AUDIOOUTPUTSETTINGSHELPER_H
