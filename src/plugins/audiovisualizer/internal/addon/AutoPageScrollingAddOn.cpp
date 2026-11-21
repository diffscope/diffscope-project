// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "AutoPageScrollingAddOn.h"

#include <coreplugin/ProjectWindowInterface.h>

#include <audio/ProjectAudioContext.h>
#include <visualeditor/ArrangementPanelInterface.h>
#include <visualeditor/AutoPageScrollingManipulator.h>
#include <visualeditor/PianoRollPanelInterface.h>

namespace AudioVisualizer::Internal {

    AutoPageScrollingAddOn::AutoPageScrollingAddOn(QObject *parent) : WindowInterfaceAddOn(parent) {
    }

    void AutoPageScrollingAddOn::initialize() {
        windowHandle()->addObject(this);
    }

    void AutoPageScrollingAddOn::extensionsInitialized() {
        const auto window = windowHandle()->cast<Core::ProjectWindowInterface>();
        const auto audioContext = Audio::ProjectAudioContext::of(window);
        const auto bind = [audioContext](VisualEditor::AutoPageScrollingManipulator *manipulator) {
            const auto syncPlaying = [audioContext, manipulator] {
                manipulator->setPlaying(audioContext->isRunning());
            };
            connect(audioContext, &Audio::ProjectAudioContext::transportStateChanged, manipulator, syncPlaying);
            syncPlaying();
        };
        if (const auto arrangement = VisualEditor::ArrangementPanelInterface::of(window))
            bind(arrangement->autoPageScrollingManipulator());
        if (const auto pianoRoll = VisualEditor::PianoRollPanelInterface::of(window))
            bind(pianoRoll->autoPageScrollingManipulator());
    }

}

#include "moc_AutoPageScrollingAddOn.cpp"
