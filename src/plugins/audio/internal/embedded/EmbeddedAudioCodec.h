// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_AUDIO_EMBEDDEDAUDIOCODEC_H
#define DIFFSCOPE_AUDIO_EMBEDDEDAUDIOCODEC_H

#include <QSharedPointer>
#include <QString>

#include <audio/EmbeddedAudio.h>

class QIODevice;
class QTemporaryFile;

namespace Audio::Internal {

    class EmbeddedAudioFile {
    public:
        EmbeddedAudioFile(QTemporaryFile &file, EmbeddedAudio::Compression compression);
        ~EmbeddedAudioFile();

        const QString path;
        const EmbeddedAudio::Compression compression;
    };

    struct EmbeddedAudioEncoded {
        QString key;
        QSharedPointer<const EmbeddedAudioFile> file;
    };

    EmbeddedAudioEncoded encodeEmbeddedAudio(const EmbeddedAudio::StreamFormat &format, const EmbeddedAudio::FrameReader &reader,
                                             EmbeddedAudio::Compression compression, double pcmScale, const std::function<bool()> &cancelled);
    bool extractWavPack(const QString &path, QIODevice &output);

}

#endif // DIFFSCOPE_AUDIO_EMBEDDEDAUDIOCODEC_H
