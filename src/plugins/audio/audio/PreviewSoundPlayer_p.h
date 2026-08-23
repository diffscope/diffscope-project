// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_AUDIO_PREVIEWSOUNDPLAYER_P_H
#define DIFFSCOPE_AUDIO_PREVIEWSOUNDPLAYER_P_H

#include <audio/PreviewSoundPlayer.h>

#include <memory>

#include <TalcsCore/TakeOwnershipPointer.h>

class QTimer;

namespace Audio {

    class PreviewSoundEndFilter;
    class PreviewSoundSource;

    class PreviewSoundPlayerPrivate {
        Q_DECLARE_PUBLIC(PreviewSoundPlayer)
    public:
        explicit PreviewSoundPlayerPrivate(talcs::AbstractAudioFormatIO *audioFormatIo, bool takeOwnership);

        void start();
        void pause();
        void handleFinished(quint64 serial);
        void syncPositionFromSource();

        PreviewSoundPlayer *q_ptr{};
        talcs::TakeOwnershipPointer<talcs::AbstractAudioFormatIO> audioFormatIo;
        std::unique_ptr<PreviewSoundSource> source;
        std::unique_ptr<PreviewSoundEndFilter> endFilter;
        QTimer *positionTimer{};
        quint64 serial{};
        bool playing{};
        double positionSecond{};
        double lengthSecond{};
    };

}

#endif // DIFFSCOPE_AUDIO_PREVIEWSOUNDPLAYER_P_H
