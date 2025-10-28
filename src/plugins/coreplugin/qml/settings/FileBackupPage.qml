import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SVSCraft
import SVSCraft.UIComponents

import DiffScope.Core

ScrollView {
    id: page

    required property QtObject pageHandle
    property bool started: false
    property int fileOption

    onFileOptionChanged: if (started) pageHandle.markDirty()

    anchors.fill: parent
    contentWidth: availableWidth

    readonly property TextMatcher matcher: TextMatcher {}

    ColumnLayout {
        width: page.width
        ColumnLayout {
            Layout.fillWidth: true
            Layout.margins: 12
            spacing: 32
            GroupBox {
                title: qsTr("File")
                TextMatcherItem on title { matcher: page.matcher }
                Layout.fillWidth: true
                ColumnLayout {
                    anchors.fill: parent
                    CheckBox {
                        text: qsTr("Lock opened files")
                        TextMatcherItem on text { matcher: page.matcher }
                        checked: page.fileOption & BehaviorPreference.FO_LockOpenedFiles
                        visible: Qt.platform.os === "windows"
                        onClicked: () => {
                            if (checked) {
                                page.fileOption |= BehaviorPreference.FO_LockOpenedFiles
                            } else {
                                page.fileOption &= ~BehaviorPreference.FO_LockOpenedFiles
                            }
                        }
                    }
                    Label {
                        ThemedItem.foregroundLevel: SVS.FL_Secondary
                        Layout.fillWidth: true
                        wrapMode: Text.Wrap
                        visible: Qt.platform.os === "windows"
                        text: qsTr("Locking an open file prevents it from being modified by other programs. Change to this option will take effect only for projects opened after the change")
                    }
                    CheckBox {
                        text: qsTr("Check for external modifications when saving a file")
                        TextMatcherItem on text { matcher: page.matcher }
                        checked: page.fileOption & BehaviorPreference.FO_CheckForExternalChangedOnSave
                        onClicked: () => {
                            if (checked) {
                                page.fileOption |= BehaviorPreference.FO_CheckForExternalChangedOnSave
                            } else {
                                page.fileOption &= ~BehaviorPreference.FO_CheckForExternalChangedOnSave
                            }
                        }
                    }
                }
            }
        }
    }
}