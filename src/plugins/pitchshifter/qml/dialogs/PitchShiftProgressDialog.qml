// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SVSCraft
import SVSCraft.UIComponents

Window {
    id: dialog

    property int stage: 0
    property real progressValue: 0
    property bool cancelling: false
    property bool closeAllowed: false
    readonly property bool isMacOS: Qt.platform.os === "osx" || Qt.platform.os === "macos"
    readonly property string statusText: {
        if (cancelling)
            return qsTr("Canceling...")
        switch (stage) {
        case 1:
            return qsTr("Analyzing...")
        case 2:
            return qsTr("Stretching and shifting pitch...")
        case 3:
            return qsTr("Finalizing...")
        default:
            return qsTr("Preparing...")
        }
    }

    width: 420
    height: windowLayout.implicitHeight
    minimumHeight: windowLayout.implicitHeight
    flags: Qt.Dialog | Qt.CustomizeWindowHint | Qt.WindowTitleHint | Qt.WindowSystemMenuHint | Qt.WindowCloseButtonHint
    modality: Qt.WindowModal
    title: qsTr("Stretch and Shift Pitch")

    signal cancelRequested()
    signal closeRequested()

    function requestCancel() {
        if (cancelling)
            return
        cancelling = true
        cancelRequested()
    }

    function done() {
        closeAllowed = true
        close()
    }

    onClosing: (close) => {
        if (closeAllowed)
            return
        close.accepted = false
        if (!cancelling)
            cancelling = true
        closeRequested()
    }

    ColumnLayout {
        id: windowLayout
        anchors.fill: parent
        spacing: 0

        Rectangle {
            color: Theme.backgroundPrimaryColor
            Layout.fillWidth: true
            implicitHeight: contentLayout.implicitHeight + (dialog.isMacOS ? 36 : 24)

            ColumnLayout {
                id: contentLayout
                anchors.fill: parent
                anchors.margins: dialog.isMacOS ? 24 : 12
                anchors.bottomMargin: 12
                spacing: 8

                Label {
                    Layout.fillWidth: true
                    text: dialog.statusText
                }

                ProgressBar {
                    Accessible.name: dialog.statusText
                    Layout.fillWidth: true
                    Layout.preferredHeight: 12
                    from: 0
                    to: 1
                    indeterminate: dialog.stage < 2
                    value: indeterminate ? 0 : dialog.progressValue
                }
            }
        }

        Rectangle {
            implicitHeight: 1
            Layout.fillWidth: true
            color: Theme.paneSeparatorColor
        }

        Rectangle {
            color: Theme.backgroundSecondaryColor
            Layout.fillWidth: true
            height: dialog.isMacOS ? 64 : 52

            RowLayout {
                anchors.fill: parent
                anchors.margins: dialog.isMacOS ? 24 : 12
                anchors.topMargin: 12

                Item {
                    Layout.fillWidth: true
                }

                Button {
                    text: qsTr("Cancel")
                    enabled: !dialog.cancelling
                    onClicked: dialog.requestCancel()
                }
            }
        }
    }
}
