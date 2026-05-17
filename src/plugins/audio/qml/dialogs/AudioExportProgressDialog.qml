import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SVSCraft
import SVSCraft.UIComponents
import SVSCraft.UIComponents.impl

Window {
    id: dialog

    property int progressStatus: ExportAudioProgressController.Preparing
    property real progressValue: 0
    property int actionStatus: ExportAudioProgressController.CancelAction
    property int alertRevision: 0
    property int runtimeWarningCount: 0
    property bool keepPartialFiles: false
    property bool closeAllowed: false
    property var messages: []
    readonly property string statusText: textOfProgressStatus(progressStatus)
    readonly property string actionButtonText: textOfActionStatus(actionStatus)
    readonly property bool keepPartialFilesVisible: isPartialFilesOptionVisible(progressStatus)

    width: 420
    height: windowLayout.implicitHeight
    minimumHeight: windowLayout.implicitHeight
    flags: Qt.Dialog | Qt.CustomizeWindowHint | Qt.WindowTitleHint | Qt.WindowSystemMenuHint | Qt.WindowCloseButtonHint
    modality: Qt.WindowModal
    title: qsTr("Export Audio")

    signal actionRequested()
    signal closeRequested()
    signal finished()

    onAlertRevisionChanged: {
        if (alertRevision > 0)
            GlobalHelper.beep()
    }

    function done() {
        closeAllowed = true
        close()
    }

    function textOfProgressStatus(status) {
        switch (status) {
        case ExportAudioProgressController.Preparing:
            return qsTr("Preparing...")
        case ExportAudioProgressController.Exporting:
            return qsTr("Exporting...")
        case ExportAudioProgressController.FinishedWithWarnings:
            return qsTr("Export finished with %Ln warning(s)", "", runtimeWarningCount)
        case ExportAudioProgressController.AbortedWithWarnings:
            return qsTr("Export aborted with %Ln warning(s)", "", runtimeWarningCount)
        case ExportAudioProgressController.FailedWithWarnings:
            return qsTr("Export failed with %Ln warning(s)", "", runtimeWarningCount)
        case ExportAudioProgressController.Aborted:
            return qsTr("Export aborted")
        case ExportAudioProgressController.Failed:
            return qsTr("Export failed")
        }
        return ""
    }

    function textOfActionStatus(status) {
        switch (status) {
        case ExportAudioProgressController.CloseAction:
            return qsTr("Close")
        case ExportAudioProgressController.CancelAction:
            return qsTr("Cancel")
        }
        return ""
    }

    function controlTypeOfStatus(status) {
        switch (status) {
        case ExportAudioProgressController.AbortedWithWarnings:
        case ExportAudioProgressController.FailedWithWarnings:
        case ExportAudioProgressController.Aborted:
        case ExportAudioProgressController.Failed:
            return SVS.CT_Error
        }
        return SVS.CT_Normal
    }

    function isPartialFilesOptionVisible(status) {
        switch (status) {
        case ExportAudioProgressController.AbortedWithWarnings:
        case ExportAudioProgressController.FailedWithWarnings:
        case ExportAudioProgressController.Aborted:
        case ExportAudioProgressController.Failed:
            return true
        }
        return false
    }

    function controlTypeOfMessage(type) {
        switch (type) {
        case ExportAudioProgressController.RuntimeErrorMessage:
            return SVS.CT_Error
        case ExportAudioProgressController.RuntimeWarningMessage:
        case ExportAudioProgressController.ClippingDetectedMessage:
            return SVS.CT_Warning
        }
        return SVS.CT_Normal
    }

    function textOfMessage(message) {
        switch (message.type) {
        case ExportAudioProgressController.ClippingDetectedMessage:
            if (message.trackNumber > 0 && message.trackName)
                return qsTr("Clipping is detected in track %L1 \"%2\"").arg(message.trackNumber).arg(message.trackName)
            if (message.trackNumber > 0)
                return qsTr("Clipping is detected in track %L1").arg(message.trackNumber)
            return qsTr("Clipping is detected")
        case ExportAudioProgressController.RuntimeErrorMessage:
            return message.text ? message.text : qsTr("Unknown error.")
        case ExportAudioProgressController.RuntimeWarningMessage:
            return message.text
        }
        return ""
    }

    onClosing: (close) => {
        if (closeAllowed) {
            finished()
            return
        }
        close.accepted = false
        closeRequested()
    }

    ColumnLayout {
        id: windowLayout
        anchors.fill: parent
        spacing: 0

        Rectangle {
            color: Theme.backgroundPrimaryColor
            Layout.fillWidth: true
            Layout.fillHeight: true
            implicitHeight: mainLayout.implicitHeight + 24

            ColumnLayout {
                id: mainLayout
                anchors.fill: parent
                anchors.margins: 12
                spacing: 8

                Label {
                    Layout.fillWidth: true
                    wrapMode: Text.Wrap
                    text: dialog.statusText
                }

                ProgressBar {
                    Layout.fillWidth: true
                    from: 0
                    to: 1
                    Layout.preferredHeight: 12
                    value: dialog.progressValue
                    ThemedItem.controlType: dialog.controlTypeOfStatus(dialog.progressStatus)
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    visible: dialog.messages.length > 0

                    Repeater {
                        model: dialog.messages

                        Annotation {
                            required property var modelData

                            Layout.fillWidth: true
                            ThemedItem.controlType: dialog.controlTypeOfMessage(modelData.type)
                            label: dialog.textOfMessage(modelData)
                        }
                    }
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
            height: 52

            RowLayout {
                anchors.fill: parent
                anchors.margins: 12

                CheckBox {
                    text: qsTr("Keep partial files")
                    visible: dialog.keepPartialFilesVisible
                    checked: dialog.keepPartialFiles
                    onToggled: dialog.keepPartialFiles = checked
                }

                Item {
                    Layout.fillWidth: true
                }

                Button {
                    text: dialog.actionButtonText
                    onClicked: dialog.actionRequested()
                }
            }
        }
    }
}
