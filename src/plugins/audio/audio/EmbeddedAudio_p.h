// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_AUDIO_EMBEDDEDAUDIO_P_H
#define DIFFSCOPE_AUDIO_EMBEDDEDAUDIO_P_H

#include <audio/EmbeddedAudio.h>

#include <QHash>
#include <QList>
#include <QSet>

namespace DspxProject {
    class Document;
}

namespace Audio {

    class EmbeddedAudioPrivate {
        Q_DECLARE_PUBLIC(EmbeddedAudio)
    public:
        struct Entry {
            QSharedPointer<const Internal::EmbeddedAudioFile> file;
            quint64 generation = 0;
        };
        using Files = QHash<QString, QSharedPointer<const Internal::EmbeddedAudioFile>>;

        EmbeddedAudio *q_ptr = nullptr;
        DspxProject::ProjectDocumentContext *context = nullptr;
        DspxProject::Document *document = nullptr;
        QHash<QString, Entry> entries;
        QList<QSharedPointer<const Internal::EmbeddedAudioFile>> retiredFiles;
        QSet<QString> savedKeys;
        quint64 generation = 0;

        void bindDocument();
        bool writeJournal(const QString &key, const Internal::EmbeddedAudioFile &file) const;
        bool publish(const Files &files);
    };

}

#endif // DIFFSCOPE_AUDIO_EMBEDDEDAUDIO_P_H
