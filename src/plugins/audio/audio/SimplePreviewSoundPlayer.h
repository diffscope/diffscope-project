// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_AUDIO_SIMPLEPREVIEWSOUNDPLAYER_H
#define DIFFSCOPE_AUDIO_SIMPLEPREVIEWSOUNDPLAYER_H

#include <QObject>

#include <audio/audioglobal.h>

namespace talcs {
    class AbstractAudioFormatIO;
}

namespace Audio {

    class SimplePreviewSoundPlayerPrivate;

    class AUDIO_EXPORT SimplePreviewSoundPlayer : public QObject {
        Q_OBJECT
        Q_DECLARE_PRIVATE(SimplePreviewSoundPlayer)

    public:
        explicit SimplePreviewSoundPlayer(talcs::AbstractAudioFormatIO *audioFormatIo, bool takeOwnership = false, QObject *parent = nullptr);
        ~SimplePreviewSoundPlayer() override;

        void play();
        void stop();

    Q_SIGNALS:
        void finished();

    private:
        QScopedPointer<SimplePreviewSoundPlayerPrivate> d_ptr;
    };

}

using SimplePreviewSoundPlayer = Audio::SimplePreviewSoundPlayer;

#endif // DIFFSCOPE_AUDIO_SIMPLEPREVIEWSOUNDPLAYER_H
