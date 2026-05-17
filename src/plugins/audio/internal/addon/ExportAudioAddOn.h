#ifndef DIFFSCOPE_AUDIO_EXPORTAUDIOADDON_H
#define DIFFSCOPE_AUDIO_EXPORTAUDIOADDON_H

#include <functional>

#include <QObject>
#include <qqmlintegration.h>

#include <CoreApi/windowinterface.h>

#include <audio/AudioExporterConfig.h>

namespace Core {
    class NotificationMessage;
    class ProjectWindowInterface;
}

namespace Audio {
    class AudioExporter;
}

namespace Audio::Internal {

    class ExportAudioProgressController : public QObject {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
    public:
        enum ProgressStatus {
            Preparing,
            Exporting,
            FinishedWithWarnings,
            AbortedWithWarnings,
            FailedWithWarnings,
            Aborted,
            Failed,
        };
        Q_ENUM(ProgressStatus)

        enum ActionStatus {
            CancelAction,
            CloseAction,
        };
        Q_ENUM(ActionStatus)

        enum MessageType {
            RuntimeWarningMessage,
            RuntimeErrorMessage,
            ClippingDetectedMessage,
        };
        Q_ENUM(MessageType)

        explicit ExportAudioProgressController(QObject *parent = nullptr);

        void setActionRequestedCallback(std::function<void()> callback);
        void setCloseRequestedCallback(std::function<void()> callback);

    public Q_SLOTS:
        void requestAction();
        void requestClose();

    private:
        std::function<void()> m_actionRequestedCallback;
        std::function<void()> m_closeRequestedCallback;
    };

    class ExportAudioAddOn : public Core::WindowInterfaceAddOn {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_PROPERTY(Audio::AudioExporterParameter currentParameter READ currentParameter WRITE setCurrentParameter NOTIFY currentParameterChanged)
    public:
        explicit ExportAudioAddOn(QObject *parent = nullptr);
        ~ExportAudioAddOn() override;

        void initialize() override;
        void extensionsInitialized() override;
        bool delayedInitialize() override;

        static ExportAudioAddOn *of(Core::ProjectWindowInterface *windowHandle);

        AudioExporterParameter currentParameter() const;
        void setCurrentParameter(const AudioExporterParameter &parameter);

        Q_INVOKABLE void exportAudio();
        Q_INVOKABLE void browseFile();
        Q_INVOKABLE bool runExport(AudioExporter *exporter);

        Q_INVOKABLE static QStringList formatOptions(int fileType) ;
        Q_INVOKABLE static void setMixingOption(int index);
        Q_INVOKABLE static void setFileType(int index);
        Q_INVOKABLE static void appendFileNameTemplate(const QString &templateString);

        Q_INVOKABLE double calculateDurationInMsec(AudioExporter *exporter) const;

    Q_SIGNALS:
        void currentParameterChanged();

    private:
        AudioExporterParameter m_currentParameter;
        Core::NotificationMessage *m_exportCompletedMessage{};
    };

}

#endif // DIFFSCOPE_AUDIO_EXPORTAUDIOADDON_H
