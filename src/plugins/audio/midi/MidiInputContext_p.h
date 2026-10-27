// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_AUDIO_MIDIINPUTCONTEXT_P_H
#define DIFFSCOPE_AUDIO_MIDIINPUTCONTEXT_P_H

#include <audio/MidiInputContext.h>

#include <atomic>
#include <memory>

#include <QStringList>

#include <TalcsMidi/AudioMidiStreamMixer.h>
#include <TalcsMidi/MidiInputDevice.h>
#include <TalcsMidi/MidiMessageIntegrator.h>
#include <TalcsMidi/MidiMessageListener.h>

#include <audio/internal/MidiPreviewStream.h>

class QTimer;

namespace Audio {

    class MidiInputContextPrivate {
        Q_DECLARE_PUBLIC(MidiInputContext)
    public:
        class ControlListener : public talcs::MidiMessageListener {
        public:
            explicit ControlListener(MidiInputContextPrivate *context);
        protected:
            bool processEvent(const talcs::MidiInputEvent &event) override;
            void processError(const QString &error) override;
        private:
            MidiInputContextPrivate *m_context;
        };

        MidiInputContext *q_ptr{};
        mutable talcs::MidiMessageListener dispatcher;
        ControlListener controlListener{this};
        Internal::MidiPreviewStream preview;
        talcs::AudioMidiStreamMixer mixer;
        talcs::MidiMessageIntegrator integrator;
        std::unique_ptr<talcs::MidiInputDevice> device;
        QStringList portNames;
        QStringList portIds;
        QVariantList devices;
        QString deviceId;
        QString previewType;
        QString error;
        double previewGain{1.0};
        bool previewEnabled{true};
        std::atomic<bool> mmcEnabled{true};
        bool initialized{};
        bool sourceRegistered{};
        QTimer *refreshTimer{};

        void reconnectDevice();
        void updatePreview();
        void dispatchControl(const talcs::MidiInputEvent &event);
        void save() const;
    };

}

#endif // DIFFSCOPE_AUDIO_MIDIINPUTCONTEXT_P_H
