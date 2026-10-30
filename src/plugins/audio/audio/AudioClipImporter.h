// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_AUDIO_AUDIOCLIPIMPORTER_H
#define DIFFSCOPE_AUDIO_AUDIOCLIPIMPORTER_H

#include <QObject>
#include <QScopedPointer>
#include <QString>
#include <QVariant>

#include <audio/audioglobal.h>

namespace Core {
    class ProjectWindowInterface;
}

namespace dspx {
    class AudioClip;
    class Track;
    struct AudioPathInfo;
}

namespace talcs {
    class AbstractAudioFormatIO;
}

namespace Audio {

    class AudioClipImporterPrivate;

    class AUDIO_EXPORT AudioClipImporter : public QObject {
        Q_OBJECT
        Q_DECLARE_PRIVATE(AudioClipImporter)
    public:
        explicit AudioClipImporter(Core::ProjectWindowInterface *windowHandle, QObject *parent = nullptr);
        ~AudioClipImporter() override;

        /** Opens a file with interactive format configuration. Returns whether preparation succeeded. */
        bool openFile(const QString &fileName);
        /** Takes ownership of io and opens it for reading. Returns whether its audio metadata and file digest are valid. */
        bool setFile(const QString &fileName, talcs::AbstractAudioFormatIO *io, const QVariant &userData, const QString &entryClassName);
        /** Returns the prepared IO, or nullptr if no file is prepared. Ownership remains with the importer until insertion. */
        talcs::AbstractAudioFormatIO *audioFormatIO() const;
        /** Returns the absolute file path, or an empty string if no file is prepared. */
        QString fileName() const;

        /** Inserts a prepared file within the caller's active transaction. Returns the clip, or nullptr on failure. */
        dspx::AudioClip *insertClip(dspx::Track *track, int position, const QString &name);

        /** Returns path metadata with the file digest and format configuration. */
        static dspx::AudioPathInfo createAudioPath(Core::ProjectWindowInterface *windowHandle, const QString &fileName, const QVariant &userData, const QString &entryClassName);
        /** Updates the path location while preserving its digest and format configuration. */
        static void updateAudioPathLocation(Core::ProjectWindowInterface *windowHandle, dspx::AudioPathInfo *path, const QString &fileName);

    private:
        QScopedPointer<AudioClipImporterPrivate> d_ptr;
    };

}

#endif // DIFFSCOPE_AUDIO_AUDIOCLIPIMPORTER_H
