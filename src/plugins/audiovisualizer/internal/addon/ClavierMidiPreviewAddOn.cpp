// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "ClavierMidiPreviewAddOn.h"

#include <QQuickItem>

#include <ScopicFlowCore/ClavierInteractionController.h>
#include <TalcsMidi/MidiInputEvent.h>
#include <TalcsMidi/MidiMessage.h>

#include <coreplugin/ProjectWindowInterface.h>

#include <audio/MidiInputContext.h>
#include <visualeditor/PianoRollPanelInterface.h>

namespace AudioVisualizer::Internal {

    ClavierMidiPreviewAddOn::ClavierMidiPreviewAddOn(QObject *parent)
        : WindowInterfaceAddOn(parent), m_sourceId(talcs::MidiInputEvent::createSourceId()) {
    }

    ClavierMidiPreviewAddOn::~ClavierMidiPreviewAddOn() {
        if (m_inputContext)
            m_inputContext->submitPreviewEvent({{}, m_sourceId, talcs::MidiInputEvent::SourceStopped});
    }

    void ClavierMidiPreviewAddOn::initialize() {
    }

    void ClavierMidiPreviewAddOn::extensionsInitialized() {
        const auto window = windowHandle()->cast<Core::ProjectWindowInterface>();
        const auto pianoRoll = VisualEditor::PianoRollPanelInterface::of(window);
        if (!pianoRoll)
            return;

        m_inputContext = Audio::MidiInputContext::instance();
        if (!m_inputContext)
            return;

        static constexpr int previewChannel = 1;
        static constexpr quint8 previewVelocity = 100;
        const auto controller = pianoRoll->clavierInteractionController();
        connect(controller, &sflow::ClavierInteractionController::pressed, this, [this](QQuickItem *, int key) {
            if (m_inputContext)
                m_inputContext->submitPreviewEvent({talcs::MidiMessage::noteOn(previewChannel, key, previewVelocity), m_sourceId});
        });
        connect(controller, &sflow::ClavierInteractionController::released, this, [this](QQuickItem *, int key) {
            if (m_inputContext)
                m_inputContext->submitPreviewEvent({talcs::MidiMessage::noteOff(previewChannel, key), m_sourceId});
        });
    }

}

#include "moc_ClavierMidiPreviewAddOn.cpp"
