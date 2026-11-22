// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef DIFFSCOPE_PITCH_SHIFTER_PITCHSHIFTADDON_H
#define DIFFSCOPE_PITCH_SHIFTER_PITCHSHIFTADDON_H

#include <QHash>
#include <QString>
#include <qqmlintegration.h>

#include <CoreApi/windowinterface.h>
#include <dspxmodelORM/AudioClip.h>

#include <audio/EmbeddedAudio.h>

class QWindow;

namespace PitchShifter::Internal {

    class PitchShiftTask;

    class PitchShiftAddOn : public Core::WindowInterfaceAddOn {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")

    public:
        enum OutputMode {
            LinkToFile,
            EmbedLossless,
            EmbedLossy,
        };
        Q_ENUM(OutputMode)

        explicit PitchShiftAddOn(QObject *parent = nullptr);
        ~PitchShiftAddOn() override;

        void initialize() override;
        void extensionsInitialized() override;
        bool delayedInitialize() override;

        Q_INVOKABLE void shiftPitch(dspx::AudioClip *clip);

    private:
        static QString sanitizedFileName(const QString &name, const QString &fallback);
        static bool execDialog(QObject *dialog);

        dspx::AudioPathInfo outputAudioPath(const QString &filePath, const QString &digest) const;
        void replaceClipAudio(dspx::AudioClip *clip, const QString &filePath, const QString &digest);
        void embedClipAudio(dspx::AudioClip *clip, const Audio::EmbeddedAudio::Prepared &prepared);
        void showCritical(const QString &title, const QString &message) const;
        void closeProgressDialog(QWindow *dialog) const;

        QHash<PitchShiftTask *, QWindow *> m_operations;
    };

}

#endif // DIFFSCOPE_PITCH_SHIFTER_PITCHSHIFTADDON_H
