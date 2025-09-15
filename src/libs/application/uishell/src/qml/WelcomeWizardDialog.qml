import QtQml
import QtQml.Models
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Controls.impl
import QtQuick.Templates as T

import SVSCraft
import SVSCraft.UIComponents
import SVSCraft.UIComponents.impl

import DiffScope.UIShell

Window {
    id: dialog
    flags: Qt.Dialog | Qt.CustomizeWindowHint | Qt.WindowTitleHint | Qt.WindowSystemMenuHint | Qt.WindowCloseButtonHint
    modality: Qt.ApplicationModal
    width: 800
    height: 500
    title: qsTr("Welcome")
    property url banner: ""
    color: Theme.backgroundPrimaryColor
    property list<Item> pages: []
    signal finished()

    onClosing: finished()

    ColumnLayout {
        anchors.fill: parent
        StackLayout {
            id: mainStackLayout
            Layout.fillWidth: true
            Layout.fillHeight: true
            Item {
                id: welcomePage
                ColumnLayout {
                    anchors.centerIn: parent
                    spacing: 32
                    Image {
                        Layout.alignment: Qt.AlignHCenter
                        source: dialog.banner
                        Layout.preferredHeight: 128
                        Layout.preferredWidth: 128 / implicitHeight * implicitWidth
                        fillMode: Image.PreserveAspectFit
                        mipmap: true
                        Accessible.role: Accessible.Graphic
                        Accessible.name: qsTr("Logo of %1").arg(Application.name)
                    }
                    Label {
                        Layout.alignment: Qt.AlignHCenter
                        text: qsTr("Welcome")
                        font.pointSize: 24
                    }
                    Label {
                        Layout.alignment: Qt.AlignHCenter
                        text: qsTr("This wizard will help you complete the initial configuration")
                        font.pointSize: 12
                    }
                }
            }
            StackView {
                id: pagesStackView
            }
        }
        Rectangle {
            Layout.fillWidth: true
            color: Theme.backgroundSecondaryColor
            implicitHeight: 60
            RowLayout {
                anchors.centerIn: parent
                spacing: 8
                Button {
                    implicitWidth: 160
                    text: mainStackLayout.currentIndex === 0 ? qsTr("Skip") : qsTr("Previous")
                    enabled: mainStackLayout.currentIndex === 0 ? true : pagesStackView.depth > 1
                    onClicked: () => {
                        if (mainStackLayout.currentIndex === 0) {
                            dialog.close()
                        } else {

                        }
                    }
                }
                Button {
                    implicitWidth: 160
                    ThemedItem.controlType: SVS.CT_Accent
                    text: mainStackLayout.currentIndex === 0 ? qsTr("Continue") : dialog.pages.length !== 0 ? qsTr("Next") : qsTr("Finish")
                    onClicked: () => {
                        if (dialog.pages.length === 0) {
                            dialog.close()
                        } else if (mainStackLayout.currentIndex === 0) {
                            mainStackLayout.currentIndex = 1
                        } else {

                        }
                    }
                }
            }
        }
    }

}