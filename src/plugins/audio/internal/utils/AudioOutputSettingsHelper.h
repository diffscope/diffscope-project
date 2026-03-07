#ifndef DIFFSCOPE_AUDIO_AUDIOOUTPUTSETTINGSHELPER_H
#define DIFFSCOPE_AUDIO_AUDIOOUTPUTSETTINGSHELPER_H

#include <qqmlintegration.h>

#include <QObject>
#include <QStringList>

namespace Audio::Internal {

    enum class ItemStatus {
        Normal,      // 正常可用的条目
        NotWorking   // 不可用的条目（用于标记当前选中但不在可用列表中的项）
    };

    struct DriverItem {
        QString name;        // 实际驱动名称（空字符串表示无可用驱动）
        ItemStatus status;   // 条目状态

        QString generateDisplayName() const;
    };

    struct DeviceItem {
        QString name;        // 实际设备名称（空字符串表示默认设备）
        ItemStatus status;   // 条目状态

        QString generateDisplayName() const;
    };

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
        Q_PROPERTY(double deviceGain READ deviceGain WRITE setDeviceGain NOTIFY deviceGainChanged)
        Q_PROPERTY(double devicePan READ devicePan WRITE setDevicePan NOTIFY devicePanChanged)

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

        double deviceGain() const;
        void setDeviceGain(double gain);

        double devicePan() const;
        void setDevicePan(double pan);

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
        void deviceGainChanged();
        void devicePanChanged();
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

        // Helper methods for structured lists
        int findDriverIndexByName(const QString &driverName, ItemStatus status = ItemStatus::Normal) const;
        int findDeviceIndexByName(const QString &deviceName, ItemStatus status = ItemStatus::Normal) const;
        void removeNotWorkingDriverItems();
        void removeNotWorkingDeviceItems();

        QList<DriverItem> m_driverItems;
        int m_driverCurrentIndex;
        QList<DeviceItem> m_deviceItems;
        int m_deviceCurrentIndex;
        QStringList m_sampleRateList;
        int m_sampleRateCurrentIndex;
        QStringList m_bufferSizeList;
        int m_bufferSizeCurrentIndex;
    };

}

#endif //DIFFSCOPE_AUDIO_AUDIOOUTPUTSETTINGSHELPER_H
