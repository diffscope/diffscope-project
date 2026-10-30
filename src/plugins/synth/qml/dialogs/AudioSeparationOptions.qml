// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SVSCraft
import SVSCraft.UIComponents

import DiffScope.Synth

ColumnLayout {
    id: options

    required property AudioExtractionAddOn addOn
    required property var trackNames
    required property real labelColumnWidth
    property string directory
    property var outputRoutes: []
    property var previousTrackNames: []
    property bool completed: false
    readonly property real directoryLabelImplicitWidth: directoryLabel.implicitWidth
    readonly property int outputTrackCount: trackCountBox.value
    readonly property bool routingValid: {
        if (trackNames.length === 0 || outputRoutes.length !== trackNames.length)
            return false
        if (outputRoutes.some(route => route < -1 || route >= outputTrackCount))
            return false
        for (let output = 0; output < outputTrackCount; ++output) {
            if (!outputRoutes.includes(output))
                return false
        }
        return true
    }
    readonly property var outputTrackNames: {
        const names = [qsTr("Do Not Output")]
        for (let index = 0; index < outputTrackCount; ++index)
            names.push(qsTr("Output %1").arg((index + 1).toLocaleString(Qt.locale(), "f", 0)))
        return names
    }

    signal controlFocused(var control)

    spacing: 12
    onTrackNamesChanged: if (completed) resetRouting()
    Component.onCompleted: {
        completed = true
        resetRouting()
    }

    function resetRouting() {
        if (trackNames.length === previousTrackNames.length
                && trackNames.every((name, index) => name === previousTrackNames[index]))
            return
        previousTrackNames = trackNames.slice()
        trackCountBox.to = Math.max(1, trackNames.length)
        trackCountBox.value = Math.max(1, trackNames.length)
        const routes = []
        for (let index = 0; index < trackNames.length; ++index)
            routes.push(index)
        outputRoutes = routes
    }

    GridLayout {
        Layout.fillWidth: true
        columns: 2
        columnSpacing: 12

        Label {
            id: directoryLabel
            Layout.minimumWidth: options.labelColumnWidth
            Layout.maximumWidth: options.labelColumnWidth
            text: qsTr("Output directory")
        }
        RowLayout {
            Layout.fillWidth: true
            TextField {
                Layout.fillWidth: true
                text: options.directory
                selectByMouse: true
                Accessible.name: directoryLabel.text
                Accessible.labelledBy: directoryLabel
                onTextEdited: options.directory = text
                onEditingFinished: options.directory = options.addOn.normalizedPath(text)
                onActiveFocusChanged: if (activeFocus) options.controlFocused(this)
            }
            ToolButton {
                text: qsTr("Browse...")
                display: AbstractButton.IconOnly
                icon.source: "image://fluent-system-icons/folder_open"
                onClicked: {
                    const directory = options.addOn.chooseDirectory(options.directory)
                    if (directory.length > 0)
                        options.directory = directory
                }
                onActiveFocusChanged: if (activeFocus) options.controlFocused(this)
            }
        }
    }

    GroupBox {
        Layout.fillWidth: true
        title: qsTr("Output Routing")

        contentItem: ColumnLayout {
            spacing: 12

            RowLayout {
                Layout.fillWidth: true
                Label {
                    id: trackCountLabel
                    Layout.fillWidth: true
                    text: qsTr("Output track count")
                    wrapMode: Text.Wrap
                }
                SpinBox {
                    id: trackCountBox
                    from: 1
                    to: 1
                    enabled: options.trackNames.length > 0
                    Accessible.name: trackCountLabel.text
                    Accessible.labelledBy: trackCountLabel
                    onValueModified: options.outputRoutes = options.outputRoutes.map(route => Math.min(route, value - 1))
                    onActiveFocusChanged: if (activeFocus) options.controlFocused(this)
                }
            }

            GridLayout {
                Layout.fillWidth: true
                columns: 2
                columnSpacing: 12
                rowSpacing: 8

                Label {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 0
                    text: qsTr("Extractor track")
                    wrapMode: Text.Wrap
                }
                Label {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 0
                    text: qsTr("Output track")
                    wrapMode: Text.Wrap
                }

                Repeater {
                    model: options.trackNames
                    delegate: RowLayout {
                        id: routeRow
                        required property int index
                        required property string modelData

                        Layout.columnSpan: 2
                        Layout.fillWidth: true
                        spacing: 12

                        Label {
                            id: sourceLabel
                            Layout.fillWidth: true
                            Layout.preferredWidth: 0
                            text: routeRow.modelData
                            textFormat: Text.PlainText
                            wrapMode: Text.Wrap
                        }
                        ComboBox {
                            Layout.fillWidth: true
                            Layout.minimumWidth: 0
                            Layout.preferredWidth: 0
                            model: options.outputTrackNames
                            currentIndex: (options.outputRoutes[routeRow.index] ?? -1) + 1
                            Accessible.name: qsTr("Output track for %1").arg(routeRow.modelData)
                            Accessible.labelledBy: sourceLabel
                            onActivated: {
                                const routes = options.outputRoutes.slice()
                                routes[routeRow.index] = currentIndex - 1
                                options.outputRoutes = routes
                            }
                            onActiveFocusChanged: if (activeFocus) options.controlFocused(this)
                        }
                    }
                }
            }

            Label {
                Layout.fillWidth: true
                visible: !options.routingValid && options.trackNames.length > 0
                text: qsTr("Assign at least one extractor track to each output track.")
                color: Theme.errorColor
                wrapMode: Text.Wrap
            }
        }
    }
}
