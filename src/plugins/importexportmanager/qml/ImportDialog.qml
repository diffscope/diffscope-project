import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SVSCraft
import SVSCraft.UIComponents

Window {
    id: window
    width: 400
    height: 600
    title: qsTr("Import")

    property string path: ""

    function browseFile() {

    }

    ColumnLayout {
        id: mainLayout
        Rectangle {
            color: Theme.backgroundQuaternaryColor
            Layout.fillWidth: true
            Layout.fillHeight: true
            ColumnLayout {
                id: contentLayout
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.margins: 12
                Label {
                    text: qsTr("Select a file to import")
                    font.pixelSize: 20
                }
                GridLayout {
                    columns: 2
                    Label {
                        text: qsTr("Path")
                    }
                    RowLayout {
                        TextField {
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
                    ComboBox {}
                }
            }
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
                }
                Button {
                    Layout.alignment: Qt.AlignVCenter
                    text: qsTr("Cancel")
                }
            }
        }
    }
}