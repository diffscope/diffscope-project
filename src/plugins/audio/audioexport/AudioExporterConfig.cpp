#include "AudioExporterConfig.h"
#include "AudioExporterConfig_p.h"

#include <QJsonValue>

namespace Audio {

    QList<int> AudioExporterParameter::source() const {
        return m_source;
    }

    void AudioExporterParameter::setSource(const QList<int> &source) {
        m_source = source;
    }

    int AudioExporterParameter::rangeStart() const {
        return m_rangeStart;
    }

    void AudioExporterParameter::setRangeStart(int rangeStart) {
        m_rangeStart = rangeStart;
    }

    int AudioExporterParameter::rangeLength() const {
        return m_rangeLength;
    }

    void AudioExporterParameter::setRangeLength(int rangeLength) {
        m_rangeLength = rangeLength;
    }

    bool AudioExporterParameter::operator==(const AudioExporterParameter &other) const {
        return m_source == other.m_source && m_rangeStart == other.m_rangeStart &&
               m_rangeLength == other.m_rangeLength;
    }

    AudioExporterConfig::AudioExporterConfig() : d(new AudioExporterConfigData) {
    }

    AudioExporterConfig::AudioExporterConfig(const AudioExporterConfig &other) = default;

    AudioExporterConfig::AudioExporterConfig(AudioExporterConfig &&other) noexcept = default;

    AudioExporterConfig &AudioExporterConfig::operator=(const AudioExporterConfig &other) = default;

    AudioExporterConfig &AudioExporterConfig::operator=(AudioExporterConfig &&other) noexcept = default;

    AudioExporterConfig::~AudioExporterConfig() = default;

    QString AudioExporterConfig::fileName() const {
        return d->fileName;
    }

    void AudioExporterConfig::setFileName(const QString &fileName) {
        d->fileName = fileName;
    }

    QString AudioExporterConfig::fileDirectory() const {
        return d->fileDirectory;
    }

    void AudioExporterConfig::setFileDirectory(const QString &fileDirectory) {
        d->fileDirectory = fileDirectory;
    }

    AudioExporterConfig::FileType AudioExporterConfig::fileType() const {
        return d->fileType;
    }

    void AudioExporterConfig::setFileType(FileType fileType) {
        d->fileType = fileType;
    }

    bool AudioExporterConfig::formatMono() const {
        return d->formatMono;
    }

    void AudioExporterConfig::setFormatMono(bool formatMono) {
        d->formatMono = formatMono;
    }

    QStringList AudioExporterConfig::formatOptionsOfType(FileType type) {
        switch (type) {
            case FT_Wav:
                return {
                    tr("32-bit float (IEEE 754)"),
                    tr("24-bit PCM"),
                    tr("16-bit PCM"),
                    tr("Unsigned 8-bit PCM"),
                };
            case FT_Flac:
                return {
                    tr("24-bit PCM"),
                    tr("16-bit PCM"),
                    tr("8-bit PCM"),
                };
            default:
                return {};
        }
    }

    QString AudioExporterConfig::extensionOfType(FileType type) {
        switch (type) {
            case FT_Wav:
                return QStringLiteral("wav");
            case FT_Flac:
                return QStringLiteral("flac");
            case FT_OggVorbis:
                return QStringLiteral("ogg");
            case FT_Mp3:
                return QStringLiteral("mp3");
        }
        return {};
    }

    int AudioExporterConfig::formatOption() const {
        return d->formatOption;
    }

    void AudioExporterConfig::setFormatOption(int formatOption) {
        d->formatOption = formatOption;
    }

    int AudioExporterConfig::formatQuality() const {
        return d->formatQuality;
    }

    void AudioExporterConfig::setFormatQuality(int formatQuality) {
        d->formatQuality = formatQuality;
    }

    double AudioExporterConfig::formatSampleRate() const {
        return d->formatSampleRate;
    }

    void AudioExporterConfig::setFormatSampleRate(double formatSampleRate) {
        d->formatSampleRate = formatSampleRate;
    }

    AudioExporterConfig::MixingOption AudioExporterConfig::mixingOption() const {
        return d->mixingOption;
    }

    void AudioExporterConfig::setMixingOption(MixingOption mixingOption) {
        d->mixingOption = mixingOption;
    }

    bool AudioExporterConfig::isMuteSoloEnabled() const {
        return d->isMuteSoloEnabled;
    }

    void AudioExporterConfig::setMuteSoloEnabled(bool enabled) {
        d->isMuteSoloEnabled = enabled;
    }

    AudioExporterConfig::SourceOption AudioExporterConfig::sourceOption() const {
        return d->sourceOption;
    }

    void AudioExporterConfig::setSourceOption(SourceOption sourceOption) {
        d->sourceOption = sourceOption;
    }

    AudioExporterConfig::TimeRange AudioExporterConfig::timeRange() const {
        return d->timeRange;
    }

    void AudioExporterConfig::setTimeRange(TimeRange timeRange) {
        d->timeRange = timeRange;
    }

    QJsonObject AudioExporterConfig::toJsonObject() const {
        return {
            {"fileName",          d->fileName                         },
            {"fileDirectory",     d->fileDirectory                    },
            {"fileType",          static_cast<int>(d->fileType)       },
            {"formatMono",        d->formatMono                       },
            {"formatOption",      d->formatOption                     },
            {"formatQuality",     d->formatQuality                    },
            {"formatSampleRate",  d->formatSampleRate                 },
            {"mixingOption",      static_cast<int>(d->mixingOption)   },
            {"isMuteSoloEnabled", d->isMuteSoloEnabled                },
            {"sourceOption",      static_cast<int>(d->sourceOption)   },
            {"timeRange",         static_cast<int>(d->timeRange)      },
        };
    }

    AudioExporterConfig AudioExporterConfig::fromJsonObject(const QJsonObject &object) {
        AudioExporterConfig config;
        const auto assignString = [&object](const QString &key, QString &out) {
            const auto value = object.value(key);
            if (value.isString()) {
                out = value.toString();
            }
        };
        const auto assignInt = [&object](const QString &key, int &out) {
            const auto value = object.value(key);
            if (value.isDouble()) {
                out = value.toInt();
            }
        };

        assignString(QStringLiteral("fileName"), config.d->fileName);
        assignString(QStringLiteral("fileDirectory"), config.d->fileDirectory);

        int fileType = config.d->fileType;
        assignInt(QStringLiteral("fileType"), fileType);
        config.d->fileType = static_cast<FileType>(fileType);

        auto value = object.value(QStringLiteral("formatMono"));
        if (value.isBool()) {
            config.d->formatMono = value.toBool();
        }

        assignInt(QStringLiteral("formatOption"), config.d->formatOption);
        assignInt(QStringLiteral("formatQuality"), config.d->formatQuality);

        value = object.value(QStringLiteral("formatSampleRate"));
        if (value.isDouble()) {
            config.d->formatSampleRate = value.toDouble();
        }

        int mixingOption = config.d->mixingOption;
        assignInt(QStringLiteral("mixingOption"), mixingOption);
        config.d->mixingOption = static_cast<MixingOption>(mixingOption);

        value = object.value(QStringLiteral("isMuteSoloEnabled"));
        if (value.isBool()) {
            config.d->isMuteSoloEnabled = value.toBool();
        }

        int sourceOption = config.d->sourceOption;
        assignInt(QStringLiteral("sourceOption"), sourceOption);
        config.d->sourceOption = static_cast<SourceOption>(sourceOption);

        int timeRange = config.d->timeRange;
        assignInt(QStringLiteral("timeRange"), timeRange);
        config.d->timeRange = static_cast<TimeRange>(timeRange);
        return config;
    }

    bool AudioExporterConfig::operator==(const AudioExporterConfig &other) const {
        return d->fileName == other.d->fileName && d->fileDirectory == other.d->fileDirectory &&
               d->fileType == other.d->fileType && d->formatMono == other.d->formatMono &&
               d->formatOption == other.d->formatOption && d->formatQuality == other.d->formatQuality &&
               d->formatSampleRate == other.d->formatSampleRate && d->mixingOption == other.d->mixingOption &&
               d->isMuteSoloEnabled == other.d->isMuteSoloEnabled && d->sourceOption == other.d->sourceOption &&
               d->timeRange == other.d->timeRange;
    }

}
