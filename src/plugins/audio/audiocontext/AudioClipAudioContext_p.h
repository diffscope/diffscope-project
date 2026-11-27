// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_AUDIO_AUDIOCLIPAUDIOCONTEXT_P_H
#define DIFFSCOPE_AUDIO_AUDIOCLIPAUDIOCONTEXT_P_H

#include <audio/AudioClipAudioContext.h>

#include <QPointer>

namespace dspx {
    struct AudioPathInfo;
}

namespace talcs {
    class DspxAudioClipContext;
    class DspxTrackContext;
}

namespace Audio {

    class AudioClipAudioContextPrivate {
        Q_DECLARE_PUBLIC(AudioClipAudioContext)
    public:
        AudioClipAudioContext *q_ptr{};
        Core::ProjectWindowInterface *windowHandle{};
        dspx::AudioClip *clip{};
        QPointer<talcs::DspxTrackContext> trackContext{};
        talcs::DspxAudioClipContext *clipContext{};
        AudioClipAudioContext::Status status = AudioClipAudioContext::Unknown;
        QString realAudioPath;
        double sourceSampleRate = 0;
        qint64 sourceSampleCount = -1;

        void setStatus(AudioClipAudioContext::Status status);
        void setRealAudioPath(const QString &path);
        void updateReadAheadSize();
        void notifyAudioChanged();

        static AudioClipAudioContext *create(Core::ProjectWindowInterface *windowHandle, dspx::AudioClip *clip, talcs::DspxTrackContext *trackContext);
        static AudioClipAudioContextPrivate *of(AudioClipAudioContext *q);
        static QString findReferencedFile(const dspx::AudioPathInfo &path, const QString &projectPath);
        static void updateReadAheadSizes();
    };

}

#endif // DIFFSCOPE_AUDIO_AUDIOCLIPAUDIOCONTEXT_P_H
