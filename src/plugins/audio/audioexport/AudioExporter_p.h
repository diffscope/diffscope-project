#ifndef DIFFSCOPE_AUDIO_AUDIOEXPORTER_P_H
#define DIFFSCOPE_AUDIO_AUDIOEXPORTER_P_H

#include <audio/AudioExporter.h>

#include <QHash>
#include <QStringList>

namespace dspx {
    class Track;
}

namespace talcs {
    class DspxProjectAudioExporter;
    class DspxProjectContext;
    class DspxTrackContext;
}

namespace Audio {

    class AudioExporterPrivate {
        Q_DECLARE_PUBLIC(AudioExporter)
    public:
        AudioExporter *q_ptr{};
        Core::ProjectWindowInterface *windowHandle{};
        AudioExporterConfig config;
        AudioExporterParameter parameter;
        bool configInitialized{};
        AudioExporter::PreflightWarnings preflightWarnings{};
        QStringList fileList;
        QStringList temporaryFileList;
        talcs::DspxProjectAudioExporter *currentExporter{};
        AudioExporter::Error error{AudioExporter::NoError};
        QString errorString;
        bool running{};

        QString projectName() const;
        QString projectDirectory() const;
        QList<dspx::Track *> tracks() const;
        QString trackName(int trackIndex) const;
        talcs::DspxProjectContext *projectContext() const;
        QPair<int, int> calculateRange() const;
        QList<int> sourceIndexes(bool *ok = nullptr) const;
        QList<talcs::DspxTrackContext *> sourceTrackContexts(const QList<int> &sourceIndexes) const;

        bool resolveTemplate(QString &templateString) const;
        bool resolveTemplate(QString &templateString, const QString &trackName, int trackIndex) const;
        int calculateFormat() const;
        void refreshPreflight();

        void setError(AudioExporter::Error error, const QString &message = {});
        void clearError();
        void setRunning(bool running);

        static AudioExporterPrivate *of(AudioExporter *q);
    };

}

#endif // DIFFSCOPE_AUDIO_AUDIOEXPORTER_P_H
