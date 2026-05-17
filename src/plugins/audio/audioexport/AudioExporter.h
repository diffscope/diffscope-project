#ifndef DIFFSCOPE_AUDIO_AUDIOEXPORTER_H
#define DIFFSCOPE_AUDIO_AUDIOEXPORTER_H

#include <QObject>
#include <QStringList>

#include <audio/AudioExporterConfig.h>
#include <audio/audioglobal.h>

namespace Core {
    class ProjectWindowInterface;
}

namespace Audio {

    namespace Internal {
        class AudioExportDialog;
    }

    class AudioExporter;
    class AudioExporterPrivate;

    class AudioExporterListener {
    public:
        virtual ~AudioExporterListener() = default;
        virtual bool willStartCallback(AudioExporter *exporter) = 0;
        virtual void willFinishCallback(AudioExporter *exporter) = 0;
    };

    class AUDIO_EXPORT AudioExporter : public QObject {
        Q_OBJECT
        Q_DECLARE_PRIVATE(AudioExporter)
        Q_PROPERTY(Core::ProjectWindowInterface *windowHandle READ windowHandle CONSTANT)
        Q_PROPERTY(Audio::AudioExporterConfig config READ config WRITE setConfig NOTIFY configChanged)
        Q_PROPERTY(Audio::AudioExporterParameter parameter READ parameter WRITE setParameter NOTIFY parameterChanged)
        Q_PROPERTY(PreflightWarnings preflightWarnings READ preflightWarnings NOTIFY preflightWarningsChanged)
        Q_PROPERTY(QStringList fileList READ fileList NOTIFY fileListChanged)
        Q_PROPERTY(Error error READ error NOTIFY errorChanged)
        Q_PROPERTY(QString errorString READ errorString NOTIFY errorChanged)
        Q_PROPERTY(bool running READ isRunning NOTIFY runningChanged)

    public:
        explicit AudioExporter(Core::ProjectWindowInterface *windowHandle, QObject *parent = nullptr);
        ~AudioExporter() override;

        Core::ProjectWindowInterface *windowHandle() const;

        static void registerListener(AudioExporterListener *listener);

        AudioExporterConfig config() const;
        void setConfig(const AudioExporterConfig &config);

        AudioExporterParameter parameter() const;
        void setParameter(const AudioExporterParameter &parameter);

        enum PreflightWarningFlag {
            PW_NoFile = 0x0001,
            PW_DuplicatedFile = 0x0002,
            PW_WillOverwrite = 0x0004,
            PW_UnrecognizedTemplate = 0x0008,
            PW_LossyFormat = 0x0010,
        };
        Q_ENUM(PreflightWarningFlag)
        Q_DECLARE_FLAGS(PreflightWarnings, PreflightWarningFlag)
        Q_FLAG(PreflightWarnings)

        PreflightWarnings preflightWarnings() const;
        Q_INVOKABLE static QStringList preflightWarningTexts(PreflightWarnings warnings);

        QStringList fileList() const;
        QStringList dryRun() const;

        enum Error {
            NoError,
            InvalidConfig,
            CannotOpenFile,
            UnsupportedFormat,
            CannotStartExport,
            ExporterFailed,
            RenameTemporaryFileFailed,
            Cancelled,
            UnknownError,
        };
        Q_ENUM(Error)

        Error error() const;
        QString errorString() const;

        bool isRunning() const;

        enum Result {
            R_Ok,
            R_Fail,
            R_Abort,
        };
        Q_ENUM(Result)

        Q_INVOKABLE Result exec();
        Q_INVOKABLE void cleanUp();
        Q_INVOKABLE void cancel(bool isFail = false, const QString &message = {});
        Q_INVOKABLE void addRuntimeWarning(const QString &message, int sourceIndex = -1);

    Q_SIGNALS:
        void configChanged();
        void parameterChanged();
        void preflightWarningsChanged();
        void fileListChanged();
        void errorChanged();
        void runningChanged();
        void progressChanged(double progressRatio, int sourceIndex);
        void clippingDetected(int sourceIndex);
        void runtimeWarningAdded(const QString &message, int sourceIndex);

    private:
        friend class Internal::AudioExportDialog;
        QScopedPointer<AudioExporterPrivate> d_ptr;
    };

}

Q_DECLARE_OPERATORS_FOR_FLAGS(Audio::AudioExporter::PreflightWarnings)

using AudioExporter = Audio::AudioExporter;

#endif // DIFFSCOPE_AUDIO_AUDIOEXPORTER_H
