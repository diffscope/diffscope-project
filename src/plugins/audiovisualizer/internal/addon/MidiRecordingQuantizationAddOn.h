// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_AUDIO_VISUALIZER_MIDIRECORDINGQUANTIZATIONADDON_H
#define DIFFSCOPE_AUDIO_VISUALIZER_MIDIRECORDINGQUANTIZATIONADDON_H

#include <CoreApi/windowinterface.h>

namespace AudioVisualizer::Internal {

    class MidiRecordingQuantizationAddOn : public Core::WindowInterfaceAddOn {
        Q_OBJECT

    public:
        explicit MidiRecordingQuantizationAddOn(QObject *parent = nullptr);
        ~MidiRecordingQuantizationAddOn() override;

        void initialize() override;
        void extensionsInitialized() override;
    };

}

#endif // DIFFSCOPE_AUDIO_VISUALIZER_MIDIRECORDINGQUANTIZATIONADDON_H
