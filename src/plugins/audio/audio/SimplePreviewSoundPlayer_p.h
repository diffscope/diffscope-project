// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_AUDIO_SIMPLEPREVIEWSOUNDPLAYER_P_H
#define DIFFSCOPE_AUDIO_SIMPLEPREVIEWSOUNDPLAYER_P_H

#include <audio/SimplePreviewSoundPlayer.h>

#include <memory>

#include <TalcsCore/TakeOwnershipPointer.h>

namespace talcs {
    class AudioFormatInputSource;
}

namespace Audio {

    class PreviewSoundFinishedFilter;

    class SimplePreviewSoundPlayerPrivate {
        Q_DECLARE_PUBLIC(SimplePreviewSoundPlayer)
    public:
        explicit SimplePreviewSoundPlayerPrivate(talcs::AbstractAudioFormatIO *audioFormatIo, bool takeOwnership);
        ~SimplePreviewSoundPlayerPrivate();

        void destroySource(talcs::AudioFormatInputSource *expectedSource = nullptr);
        void finish(talcs::AudioFormatInputSource *source, quint64 serial);

        SimplePreviewSoundPlayer *q_ptr{};
        talcs::TakeOwnershipPointer<talcs::AbstractAudioFormatIO> audioFormatIo;
        std::unique_ptr<PreviewSoundFinishedFilter> finishedFilter;
        std::unique_ptr<talcs::AudioFormatInputSource> source;
        quint64 serial{};
    };

}

#endif // DIFFSCOPE_AUDIO_SIMPLEPREVIEWSOUNDPLAYER_P_H
