// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef DIFFSCOPE_PITCH_SHIFTER_PITCHSHIFTTASK_H
#define DIFFSCOPE_PITCH_SHIFTER_PITCHSHIFTTASK_H

#include <atomic>
#include <memory>

#include <QString>
#include <QThread>

namespace talcs {
    class AbstractAudioFormatIO;
}

namespace PitchShifter::Internal {

    struct PitchShiftConfig {
        enum FormantMode {
            ShiftWithPitch,
            Preserve,
            Custom,
        };

        QString outputFilePath;
        double pitchSemitones{};
        FormantMode formantMode{ShiftWithPitch};
        double formantShiftSemitones{};
        double timeRatio{};
        bool linkChannels{true};
    };

    class PitchShiftTask : public QThread {
        Q_OBJECT

    public:
        enum Stage {
            Preparing,
            Analyzing,
            Processing,
            Finalizing,
        };
        Q_ENUM(Stage)

        PitchShiftTask(std::unique_ptr<talcs::AbstractAudioFormatIO> inputIo, PitchShiftConfig config, QObject *parent = nullptr);
        ~PitchShiftTask() override;

    public Q_SLOTS:
        void requestCancel();

    Q_SIGNALS:
        void progressChanged(int stage, double progress);
        void succeeded(const QString &digest);
        void cancelled();
        void failed(const QString &message);

    protected:
        void run() override;

    private:
        std::unique_ptr<talcs::AbstractAudioFormatIO> m_inputIo;
        PitchShiftConfig m_config;
        std::atomic_bool m_cancelRequested{};
    };

}

#endif // DIFFSCOPE_PITCH_SHIFTER_PITCHSHIFTTASK_H
