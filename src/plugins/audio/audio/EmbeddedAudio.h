// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_AUDIO_EMBEDDEDAUDIO_H
#define DIFFSCOPE_AUDIO_EMBEDDEDAUDIO_H

#include <functional>

#include <QFuture>
#include <QList>
#include <QObject>
#include <QScopedPointer>
#include <QSharedPointer>
#include <QString>
#include <QVariant>

#include <dspxmodelORM/AudioClip.h>

#include <audio/audioglobal.h>

namespace DspxProject {
    class ClipboardEditScenario;
    class ProjectDocumentContext;
}

namespace Audio {

    namespace Internal {
        class AudioClipAddOn;
        class EmbeddedAudioFile;
        class EmbeddedAudioPersistenceHook;
    }

    class AudioClipAudioContext;
    class EmbeddedAudioPrivate;

    class AUDIO_EXPORT EmbeddedAudio : public QObject {
        Q_OBJECT
        Q_DECLARE_PRIVATE(EmbeddedAudio)
    public:
        enum class Compression { Lossless, Lossy };
        enum class SampleType { Int8, Int16, Int24, Float32, Other };

        struct StreamFormat {
            int sampleRate = 0;
            int channelCount = 0;
            qint64 frameCount = -1;
            SampleType sampleType = SampleType::Float32;
        };

        struct Prepared {
            QString key() const { return m_key; }
            QString error() const { return m_error; }
            Compression compression() const { return m_compression; }

        private:
            friend class EmbeddedAudio;
            QString m_key;
            QString m_error;
            Compression m_compression = Compression::Lossless;
            QSharedPointer<const Internal::EmbeddedAudioFile> m_file;
        };

        struct Conflict {
            enum Action { Add, Reuse, Upgrade, Downgrade };
            Action action = Add;
            QString key;
            qsizetype referenceCount = 0;
            quint64 generation = 0;
        };

        enum class ApplyResult { Applied, NoChange, Busy, Stale, DowngradeRequired, WriteFailed };

        using FrameReader = std::function<qint64(float *, qint64)>;

        ~EmbeddedAudio() override;

        static EmbeddedAudio *of(DspxProject::ProjectDocumentContext *context);
        /** Encodes frames produced by reader on a worker thread. The reader returns zero at end of stream. */
        static QFuture<Prepared> prepare(const StreamFormat &format, FrameReader reader, Compression compression);
        /** Returns the current managed audio file path for key, or an empty path if unavailable. */
        QString filePath(const QString &key) const;
        /** Returns the compression action for the prepared audio. */
        Conflict conflict(const Prepared &prepared) const;
        /** Applies prepared audio and calls edit inside a document transaction. Returns NoChange if edit returns false. */
        ApplyResult apply(const QList<Prepared> &prepared, const QList<Conflict> &expected, const QString &transactionName,
                          const std::function<bool(const QList<dspx::AudioPathInfo> &)> &edit, bool allowDowngrade = false);

    Q_SIGNALS:
        void contentChanged(const QString &key);

    private:
        friend class Internal::EmbeddedAudioPersistenceHook;
        friend class Internal::AudioClipAddOn;
        friend class AudioClipAudioContext;
        friend class EmbeddedAudioPrivate;

        explicit EmbeddedAudio(DspxProject::ProjectDocumentContext *context);
        static QFuture<Prepared> prepare(const QString &path, const QVariant &userData, const QString &formatEntry, Compression compression);
        bool isLossless(const QString &key) const;
        bool extractToFile(const QString &key, const QString &filePath) const;
        void configureClipboard(DspxProject::ClipboardEditScenario *scenario, const std::function<void()> &reportCopyFailure);
        QScopedPointer<EmbeddedAudioPrivate> d_ptr;
    };

}

#endif // DIFFSCOPE_AUDIO_EMBEDDEDAUDIO_H
