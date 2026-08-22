// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

pragma ComponentBehavior: Bound

import QtQml
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

import SVSCraft
import SVSCraft.UIComponents

import DiffScope.UIShell
import DiffScope.VisualEditor

WelcomeWizardPage {
    id: page

    title: qsTr("Move and Zoom")
    description: qsTr("Configure the moving and zooming behavior of editor's viewport, and try it out in the playground")

    readonly property var hints: EditorInteractionHelper.scrollBehaviorHints(
        EditorPreference.alternateAxisModifier,
        EditorPreference.zoomModifier,
        EditorPreference.pageModifier,
        EditorPreference.usePageModifierAsAlternateAxisZoom,
        EditorPreference.middleButtonAutoScroll)

    RowLayout {
        anchors.fill: parent
        anchors.margins: 32
        spacing: 32

        ColumnLayout {
            id: settingsPane

            Layout.fillHeight: true
            // Lock the settings pane to the fixed width of its contents so that
            // it never gets resized by the playground pane contents.
            Layout.preferredWidth: settingsPane.implicitWidth
            Layout.maximumWidth: settingsPane.implicitWidth
            spacing: 16

            GridLayout {
                Layout.fillWidth: true
                columns: 3
                columnSpacing: 8
                rowSpacing: 16

                Label {
                    text: qsTr("Horizontal scroll")
                }
                Item {
                    Layout.fillWidth: true
                }
                ComboBox {
                    model: EditorInteractionHelper.scrollModifierTexts
                    currentIndex: EditorPreference.alternateAxisModifier
                    onActivated: index => {
                        EditorPreference.alternateAxisModifier = index
                        EditorPreference.save()
                    }
                }

                Label {
                    text: qsTr("Zoom")
                }
                Item {
                    Layout.fillWidth: true
                }
                ComboBox {
                    model: EditorInteractionHelper.scrollModifierTexts
                    currentIndex: EditorPreference.zoomModifier
                    onActivated: index => {
                        EditorPreference.zoomModifier = index
                        EditorPreference.save()
                    }
                }

                RowLayout {
                    RadioButton {
                        text: qsTr("Scroll by page")
                        checked: !EditorPreference.usePageModifierAsAlternateAxisZoom
                        onClicked: {
                            EditorPreference.usePageModifierAsAlternateAxisZoom = false
                            EditorPreference.save()
                        }
                    }
                    RadioButton {
                        text: qsTr("Horizontal zoom")
                        checked: EditorPreference.usePageModifierAsAlternateAxisZoom
                        onClicked: {
                            EditorPreference.usePageModifierAsAlternateAxisZoom = true
                            EditorPreference.save()
                        }
                    }
                }
                Item {
                    Layout.fillWidth: true
                }
                ComboBox {
                    model: EditorInteractionHelper.scrollModifierTexts
                    currentIndex: EditorPreference.pageModifier
                    onActivated: index => {
                        EditorPreference.pageModifier = index
                        EditorPreference.save()
                    }
                }

                Label {
                    text: qsTr("Middle button/hand tool scroll mode")
                }
                Item {
                    Layout.fillWidth: true
                }
                ComboBox {
                    model: [qsTr("Dragging"), qsTr("Auto scrolling")]
                    currentIndex: EditorPreference.middleButtonAutoScroll ? 1 : 0
                    onActivated: index => {
                        EditorPreference.middleButtonAutoScroll = (index === 1)
                        EditorPreference.save()
                    }
                }
            }

            GroupBox {
                title: qsTr("Playground")
                Layout.fillWidth: true
                Layout.fillHeight: true

                MoveAndZoomPreview {
                    anchors.fill: parent
                }
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true

            Item {
                Layout.fillHeight: true
            }
            Repeater {
                model: page.hints
                delegate: RowLayout {
                    id: hintDelegate

                    required property var modelData
                    required property int index

                    Layout.fillWidth: true
                    Layout.topMargin: index === 8 ? 12 : 0

                    Label {
                        text: hintDelegate.modelData.combination
                    }
                    Label {
                        Layout.fillWidth: true
                        ThemedItem.foregroundLevel: SVS.FL_Secondary
                        horizontalAlignment: Text.AlignRight
                        wrapMode: Text.Wrap
                        text: hintDelegate.modelData.behavior
                    }
                }
            }
            Item {
                Layout.fillHeight: true
            }
        }
    }
}
