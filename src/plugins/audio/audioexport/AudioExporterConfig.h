#ifndef DIFFSCOPE_AUDIO_AUDIOEXPORTERCONFIG_H
#define DIFFSCOPE_AUDIO_AUDIOEXPORTERCONFIG_H

#include <QCoreApplication>
#include <QJsonObject>
#include <QList>
#include <QSharedDataPointer>
#include <QString>
#include <QStringList>

#include <audio/audioglobal.h>

namespace Audio {

    class AudioExporterConfigData;

    class AUDIO_EXPORT AudioExporterParameter {
        Q_GADGET
        Q_PROPERTY(QList<int> source READ source WRITE setSource)
        Q_PROPERTY(int rangeStart READ rangeStart WRITE setRangeStart)
        Q_PROPERTY(int rangeLength READ rangeLength WRITE setRangeLength)
    public:
        QList<int> source() const;
        void setSource(const QList<int> &source);

        int rangeStart() const;
        void setRangeStart(int rangeStart);

        int rangeLength() const;
        void setRangeLength(int rangeLength);

        bool operator==(const AudioExporterParameter &other) const;

    private:
        QList<int> m_source;
        int m_rangeStart{};
        int m_rangeLength{};
    };

    class AUDIO_EXPORT AudioExporterConfig {
        Q_GADGET
        Q_PROPERTY(QString fileName READ fileName WRITE setFileName)
        Q_PROPERTY(QString fileDirectory READ fileDirectory WRITE setFileDirectory)
        Q_PROPERTY(FileType fileType READ fileType WRITE setFileType)
        Q_PROPERTY(bool formatMono READ formatMono WRITE setFormatMono)
        Q_PROPERTY(int formatOption READ formatOption WRITE setFormatOption)
        Q_PROPERTY(int formatQuality READ formatQuality WRITE setFormatQuality)
        Q_PROPERTY(double formatSampleRate READ formatSampleRate WRITE setFormatSampleRate)
        Q_PROPERTY(MixingOption mixingOption READ mixingOption WRITE setMixingOption)
        Q_PROPERTY(bool muteSoloEnabled READ isMuteSoloEnabled WRITE setMuteSoloEnabled)
        Q_PROPERTY(SourceOption sourceOption READ sourceOption WRITE setSourceOption)
        Q_PROPERTY(TimeRange timeRange READ timeRange WRITE setTimeRange)
        Q_DECLARE_TR_FUNCTIONS(Audio::AudioExporterConfig)
    public:
        AudioExporterConfig();
        AudioExporterConfig(const AudioExporterConfig &other);
        AudioExporterConfig(AudioExporterConfig &&other) noexcept;
        AudioExporterConfig &operator=(const AudioExporterConfig &other);
        AudioExporterConfig &operator=(AudioExporterConfig &&other) noexcept;
        ~AudioExporterConfig();

        QString fileName() const;
        void setFileName(const QString &fileName);

        QString fileDirectory() const;
        void setFileDirectory(const QString &fileDirectory);

        enum FileType {
            FT_Wav,
            FT_Flac,
            FT_OggVorbis,
            FT_Mp3,
        };
        Q_ENUM(FileType)
        FileType fileType() const;
        void setFileType(FileType fileType);

        bool formatMono() const;
        void setFormatMono(bool formatMono);

        static QStringList formatOptionsOfType(FileType type);
        static QString extensionOfType(FileType type);

        int formatOption() const;
        void setFormatOption(int formatOption);

        int formatQuality() const;
        void setFormatQuality(int formatQuality);

        double formatSampleRate() const;
        void setFormatSampleRate(double formatSampleRate);

        enum MixingOption {
            MO_Mixed,
            MO_Separated,
            MO_SeparatedThruMaster,
        };
        Q_ENUM(MixingOption)
        MixingOption mixingOption() const;
        void setMixingOption(MixingOption mixingOption);

        bool isMuteSoloEnabled() const;
        void setMuteSoloEnabled(bool enabled);

        enum SourceOption {
            SO_All,
            SO_Selected,
            SO_Custom,
        };
        Q_ENUM(SourceOption)
        SourceOption sourceOption() const;
        void setSourceOption(SourceOption sourceOption);

        enum TimeRange {
            TR_All,
            TR_LoopSection,
            TR_Custom,
        };
        Q_ENUM(TimeRange)
        TimeRange timeRange() const;
        void setTimeRange(TimeRange timeRange);

        QJsonObject toJsonObject() const;
        [[nodiscard]] static AudioExporterConfig fromJsonObject(const QJsonObject &object);

        bool operator==(const AudioExporterConfig &other) const;

    private:
        QSharedDataPointer<AudioExporterConfigData> d;
    };

}

#endif // DIFFSCOPE_AUDIO_AUDIOEXPORTERCONFIG_H
