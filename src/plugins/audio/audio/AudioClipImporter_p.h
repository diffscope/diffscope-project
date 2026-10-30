// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_AUDIO_AUDIOCLIPIMPORTER_P_H
#define DIFFSCOPE_AUDIO_AUDIOCLIPIMPORTER_P_H

#include <audio/AudioClipImporter.h>

#include <memory>

#include <QPointer>

#include <TalcsFormat/AbstractAudioFormatIO.h>

#include <dspxmodelORM/AudioClip.h>

namespace Core {
    class DspxDocument;
}

namespace Audio {

    class AudioClipImporterPrivate {
        Q_DECLARE_PUBLIC(AudioClipImporter)
    public:
        AudioClipImporter *q_ptr{};
        QPointer<Core::ProjectWindowInterface> windowHandle;
        QPointer<Core::DspxDocument> document;
        std::unique_ptr<talcs::AbstractAudioFormatIO> io;
        QString fileName;
        dspx::AudioPathInfo path;
        double durationMsec{};
    };

}

#endif // DIFFSCOPE_AUDIO_AUDIOCLIPIMPORTER_P_H
