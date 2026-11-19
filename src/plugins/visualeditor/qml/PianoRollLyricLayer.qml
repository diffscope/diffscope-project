// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls

import dev.sjimo.ScopicFlow

import SVSCraft
import SVSCraft.UIComponents

import DiffScope.Core
import DiffScope.DspxModel as DspxModel

Item {
    id: layer

    readonly property ProjectWindowInterface windowHandle: ViewLayer.windowHandle
    readonly property ProjectViewModelContext context: windowHandle?.ProjectViewModelContext.context ?? null
    readonly property PianoRollPanelInterface pianoRoll: ViewLayer.pianoRollPanelInterface
    readonly property DspxModel.SingingClip editingClip: pianoRoll?.editingClip ?? null
    readonly property ClipViewModel clipViewModel: context?.getClipViewItemFromDocumentItem(editingClip) ?? null
    readonly property QtObject transactionController: windowHandle?.projectDocumentContext.document?.transactionController ?? null
    readonly property bool canFillLyrics: transactionController?.canBeginTransaction === true && transactionController.canAcquireReadLock
    readonly property rect viewport: ViewLayer.visibleRect
    readonly property real timeScale: pianoRoll?.timeLayoutViewModel.pixelDensity ?? 0
    readonly property real clipTimeStart: (pianoRoll?.timeViewModel.start ?? 0)
        - (clipViewModel?.position ?? 0) + (clipViewModel?.clipStart ?? 0)
    readonly property real keyStart: pianoRoll?.clavierViewModel.start ?? 0
    readonly property real keyHeight: pianoRoll?.clavierViewModel.pixelDensity ?? 0
    readonly property real lyricOffset: 24
    readonly property real lyricPadding: 6
    readonly property real lyricHeight: fontMetrics.height + 2 * lyricPadding

    z: -1
    visible: EditorPreference.showLyricOverlay && editingClip !== null

    function fillPhrase(firstNote: NoteViewModel): void {
        if (visible && canFillLyrics && phrases.selectPhrase(firstNote, context?.noteSelectionController ?? null))
            windowHandle.fillLyricsScenario.fillLyrics(windowHandle.window)
    }

    FontMetrics {
        id: fontMetrics
        font: Theme.font
    }

    LyricPhraseModel {
        id: phrases

        noteSequence: layer.visible ? layer.clipViewModel?.associatedNoteSequence ?? null : null
        gapThreshold: EditorPreference.lyricPhraseGapThreshold
        minimumDuration: layer.timeScale > 0 ? 2 * layer.lyricPadding / layer.timeScale : 0
        visibleRange: {
            const viewport = layer.viewport
            if (!layer.visible || layer.timeScale <= 0 || layer.keyHeight <= 0 || viewport.width <= 0 || viewport.height <= 0)
                return Qt.rect(0, 0, 0, 0)
            return Qt.rect(layer.clipTimeStart + viewport.x / layer.timeScale,
                layer.keyStart - (viewport.y + viewport.height - layer.lyricOffset) / layer.keyHeight,
                viewport.width / layer.timeScale, (viewport.height + layer.lyricHeight) / layer.keyHeight)
        }
    }

    // The note layer receives pointer input across the editor scene.
    Item {
        id: inputViewport

        parent: layer.parent
        z: 0.5
        x: layer.viewport.x
        y: layer.viewport.y
        width: layer.viewport.width
        height: layer.viewport.height
        clip: true
        visible: layer.visible
        enabled: layer.enabled && layer.canFillLyrics
    }

    Item {
        x: layer.viewport.x
        y: layer.viewport.y
        width: layer.viewport.width
        height: layer.viewport.height
        clip: true

        Repeater {
            model: phrases

            delegate: Rectangle {
                id: phrase

                required property NoteViewModel firstNote
                required property int startTick
                required property real endTick
                required property int lowestKey
                required property string lyrics
                property real hoverProgress: input.containsMouse ? 1 : 0

                x: (startTick - layer.clipTimeStart) * layer.timeScale - layer.viewport.x
                y: (layer.keyStart - lowestKey) * layer.keyHeight + layer.lyricOffset - layer.viewport.y
                width: Math.min(lyricLabel.implicitWidth, Math.max(0, (endTick - startTick) * layer.timeScale))
                height: layer.lyricHeight
                clip: true
                color: {
                    const background = Theme.controlColor(SVS.CT_Normal)
                    return Qt.rgba(background.r, background.g, background.b, hoverProgress)
                }
                border.color: {
                    const stroke = Theme.borderColor
                    return Qt.rgba(stroke.r, stroke.g, stroke.b, stroke.a * hoverProgress)
                }

                Behavior on hoverProgress {
                    NumberAnimation {
                        duration: Theme.colorAnimationDuration
                        easing.type: Easing.OutCubic
                    }
                }

                Label {
                    id: lyricLabel

                    anchors.fill: parent
                    padding: layer.lyricPadding
                    text: phrase.lyrics
                    textFormat: Text.PlainText
                    maximumLineCount: 1
                    elide: Text.ElideRight
                    color: Theme.foregroundPrimaryColor
                }

                MouseArea {
                    id: input

                    parent: inputViewport
                    x: phrase.x
                    y: phrase.y
                    width: phrase.width
                    height: phrase.height
                    hoverEnabled: true
                    containmentMask: QtObject {
                        function contains(position: point): bool {
                            if (position.x < 0 || position.x >= input.width || position.y < 0 || position.y >= input.height)
                                return false
                            const tick = layer.clipTimeStart + (input.x + position.x + layer.viewport.x) / layer.timeScale
                            const key = Math.ceil(layer.keyStart - (input.y + position.y + layer.viewport.y) / layer.keyHeight) - 1
                            const notes = layer.editingClip?.notes.slice(Math.floor(tick), 1) ?? []
                            for (const note of notes) {
                                if (note.keyNumber === key)
                                    return false
                            }
                            return true
                        }
                    }

                    onClicked: Qt.callLater(layer.fillPhrase, phrase.firstNote)
                }
            }
        }
    }
}
