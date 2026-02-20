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
    property list<Item> pagesWithHeader: []
    signal finished()

    onClosing: finished()

    Component {
        id: pageWithHeaderComponent
        Item {
            id: pageWithHeader
            required property Item page
            Rectangle {
                anchors.fill: parent
                color: Theme.backgroundPrimaryColor
            }
            ColumnLayout {
                anchors.fill: parent
                spacing: 4
                Label {
                    Layout.alignment: Qt.AlignHCenter
                    Layout.topMargin: 8
                    text: pageWithHeader.page.title
                    font.pixelSize: 20
                }
                Label {
                    Layout.alignment: Qt.AlignHCenter
                    text: pageWithHeader.page.description
                    font.pixelSize: 16
                }
                StackLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    data: [pageWithHeader.page]
                }
            }
        }
    }

    Component.onCompleted: () => {
        for (let page of pages) {
            let pageWithHeader = pageWithHeaderComponent.createObject(null, {page})
            pagesWithHeader.push(pageWithHeader)
        }
    }

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
                        Accessible.name: qsTr("Logo of %1").arg(Application.displayName)
                    }
                    Label {
                        Layout.alignment: Qt.AlignHCenter
                        text: qsTr("Welcome")
                        font.pixelSize: 32
                    }
                    Label {
                        Layout.alignment: Qt.AlignHCenter
                        text: qsTr("This wizard will help you complete the initial configuration")
                        font.pixelSize: 16
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
                            let page = pagesStackView.pop()
                            dialog.pagesWithHeader.unshift(page)
                        }
                    }
                }
                Button {
                    implicitWidth: 160
                    ThemedItem.controlType: SVS.CT_Accent
                    text: mainStackLayout.currentIndex === 0 ? qsTr("Continue") : dialog.pagesWithHeader.length !== 0 ? qsTr("Next") : qsTr("Finish")
                    onClicked: () => {
                        if (dialog.pagesWithHeader.length === 0) {
                            dialog.close()
                            return
                        }
                        if (mainStackLayout.currentIndex === 0) {
                            mainStackLayout.currentIndex = 1
                        }
                        let page = dialog.pagesWithHeader.shift()
                        pagesStackView.push(page)
                    }
                }
            }
        }
    }

}