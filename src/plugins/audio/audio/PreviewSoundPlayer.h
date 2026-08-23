// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_AUDIO_PREVIEWSOUNDPLAYER_H
#define DIFFSCOPE_AUDIO_PREVIEWSOUNDPLAYER_H

#include <QObject>

#include <audio/audioglobal.h>

namespace talcs {
    class AbstractAudioFormatIO;
}

namespace Audio {

    class PreviewSoundPlayerPrivate;

    class AUDIO_EXPORT PreviewSoundPlayer : public QObject {
        Q_OBJECT
        Q_DECLARE_PRIVATE(PreviewSoundPlayer)
        Q_PROPERTY(bool playing READ isPlaying WRITE setPlaying NOTIFY playingChanged)
        Q_PROPERTY(double positionSecond READ positionSecond WRITE setPositionSecond NOTIFY positionSecondChanged)
        Q_PROPERTY(double lengthSecond READ lengthSecond CONSTANT)

    public:
        explicit PreviewSoundPlayer(talcs::AbstractAudioFormatIO *audioFormatIo, bool takeOwnership = false, QObject *parent = nullptr);
        ~PreviewSoundPlayer() override;

        bool isPlaying() const;
        void setPlaying(bool playing);

        double positionSecond() const;
        void setPositionSecond(double positionSecond);

        double lengthSecond() const;

    Q_SIGNALS:
        void playingChanged(bool playing);
        void positionSecondChanged(double positionSecond);

    private:
        QScopedPointer<PreviewSoundPlayerPrivate> d_ptr;
    };

}

using PreviewSoundPlayer = Audio::PreviewSoundPlayer;

#endif // DIFFSCOPE_AUDIO_PREVIEWSOUNDPLAYER_H
