import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SVSCraft
import SVSCraft.UIComponents

Window {
    id: window
    width: 400
    height: 500
    flags: Qt.Dialog
    modality: Qt.ApplicationModal
    title: isExport ? qsTr("Export") : qsTr("Import")

    property bool isExport: false
    property string path: ""
    property FileConverter selectedConverter: null
    property bool accepted: false

    signal done()

    onClosing: done()

    function browseFile() {

    }

    ImportExportHelper {
        id: helper
    }

    ColumnLayout {
        id: mainLayout
        anchors.fill: parent
        spacing: 0
        Rectangle {
            color: Theme.backgroundQuaternaryColor
            Layout.fillWidth: true
            Layout.fillHeight: true
            ColumnLayout {
                id: contentLayout
                anchors.fill: parent
                anchors.margins: 12
                spacing: 12
                Label {
                    text: window.isExport ? qsTr("Select a file to export") : qsTr("Select a file to import")
                    font.pixelSize: 20
                }
                GridLayout {
                    columns: 2
                    Layout.fillWidth: true
                    Label {
                        text: qsTr("Path")
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        TextField {
                            Layout.fillWidth: true
                            text: window.path
                            onTextEdited: window.path = text
                        }
                        Button {
                            text: qsTr("Browse")
                            onClicked: window.browseFile()
                        }
                    }
                    Label {
                        text: qsTr("Format")
                    }
                    ComboBox {
                        Layout.fillWidth: true
                        model: window.isExport ? helper.exportFormatComboBoxModel : helper.importFormatComboBoxModel
                        onCurrentIndexChanged: window.selectedConverter = currentValue ?? null
                    }
                    Item {}
                    Label {
                        Layout.fillWidth: true
                        ThemedItem.foregroundLevel: SVS.FL_Secondary
                        text: window.selectedConverter?.description ?? ""
                    }
                }
                Item {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                }
            }
        }
        Rectangle {
            color: Theme.paneSeparatorColor
            implicitHeight: 1
            Layout.fillWidth: true
        }
        Rectangle {
            color: Theme.backgroundSecondaryColor
            Layout.fillWidth: true
            height: 52
            RowLayout {
                id: buttonLayout
                anchors.fill: parent
                anchors.margins: 12
                spacing: 12
                Item {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                }
                Button {
                    ThemedItem.controlType: SVS.CT_Accent
                    Layout.alignment: Qt.AlignVCenter
                    text: qsTr("Continue")
                    enabled: window.path !== "" && window.selectedConverter !== null
                    onClicked: {
                        window.accepted = true
                        window.done()
                    }
                }
                Button {
                    Layout.alignment: Qt.AlignVCenter
                    text: qsTr("Cancel")
                }
            }
        }
    }
}