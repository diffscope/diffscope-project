// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "MidiRecordingQuantizationAddOn.h"

#include <QVariant>

#include <ScopicFlowCore/TimeLayoutViewModel.h>

#include <coreplugin/ProjectWindowInterface.h>

#include <audio/ProjectAudioContext.h>
#include <visualeditor/PianoRollPanelInterface.h>
#include <visualeditor/PositionAlignmentManipulator.h>

namespace AudioVisualizer::Internal {

    MidiRecordingQuantizationAddOn::MidiRecordingQuantizationAddOn(QObject *parent) : WindowInterfaceAddOn(parent) {
    }

    MidiRecordingQuantizationAddOn::~MidiRecordingQuantizationAddOn() = default;

    void MidiRecordingQuantizationAddOn::initialize() {
        windowHandle()->addObject(this);
    }

    void MidiRecordingQuantizationAddOn::extensionsInitialized() {
        auto window = windowHandle()->cast<Core::ProjectWindowInterface>();
        connect(Audio::ProjectAudioContext::of(window), &Audio::ProjectAudioContext::midiRecordingQuantizationDialogCreated, this, [window](QObject *dialog) {
            const auto pianoRoll = VisualEditor::PianoRollPanelInterface::of(window);
            if (!pianoRoll)
                return;
            const auto manipulator = pianoRoll->positionAlignmentManipulator();
            const auto temporarilyDisabled = pianoRoll->isSnapTemporarilyDisabled();
            if (manipulator->duration() == VisualEditor::PositionAlignmentManipulator::Unset && !temporarilyDisabled)
                return;

            const auto tuplet = static_cast<int>(manipulator->tuplet());
            auto duration = static_cast<int>(manipulator->duration());
            if (duration == VisualEditor::PositionAlignmentManipulator::Auto || temporarilyDisabled) {
                const auto layout = pianoRoll->timeLayoutViewModel();
                const auto alignment = temporarilyDisabled ? layout->displayPositionAlignment() : layout->positionAlignment();
                duration = alignment * tuplet / 2;
            }
            const auto wholeNote = static_cast<int>(VisualEditor::PositionAlignmentManipulator::Note1st);
            if (duration <= 0 || wholeNote % duration != 0)
                return;
            const auto denominator = wholeNote / duration;
            if (denominator < 1 || denominator > 128 || (denominator & (denominator - 1)) != 0)
                return;

            dialog->setProperty("initialDenominator", denominator);
            dialog->setProperty("tuplet", tuplet);
        });
    }

}

#include "moc_MidiRecordingQuantizationAddOn.cpp"
