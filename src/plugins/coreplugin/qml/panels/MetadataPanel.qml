import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SVSCraft
import SVSCraft.UIComponents

import DiffScope.UIShell

QtObject {
    id: d
    required property QtObject addOn
    readonly property QtObject fileLocker: d.addOn?.windowHandle.projectDocumentContext.fileLocker ?? null

    // TODO add these components to SVSCraft
    component SelectableLabel: TextEdit {
        readOnly: true
        color: Theme.foregroundColor(ThemedItem.foregroundLevel)
        Accessible.role: Accessible.StaticText
        Accessible.name: text
        selectionColor: Theme.accentColor
    }
    component InfoCard: Frame {
        id: card
        Layout.fillWidth: true
        padding: 8
        property string title: ""
        property string text: ""
        background: Rectangle {
            color: Theme.backgroundPrimaryColor
            border.width: 1
            border.color: Theme.borderColor
            radius: 4
        }
        ColumnLayout {
            spacing: 4
            anchors.fill: parent
            Label {
                text: card.title
                font.pixelSize: 14
                font.weight: Font.DemiBold
            }
            SelectableLabel {
                Layout.fillWidth: true
                ThemedItem.foregroundLevel: SVS.FL_Secondary
                text: card.text
                wrapMode: Text.Wrap
            }
        }
    }

    readonly property Component metadataPanelComponent: ActionDockingPane {
        id: pane
        header: Item {
            anchors.fill: parent
            ToolButton {
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                icon.source: "image://fluent-system-icons/edit"
                text: qsTr("Edit")
                onClicked: () => {

                }
            }
        }
        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 16
            spacing: 0
            ScrollView {
                id: scrollView
                Layout.fillWidth: true
                Layout.fillHeight: true
                contentWidth: availableWidth
                ColumnLayout {
                    width: scrollView.width
                    ColumnLayout {
                        Layout.fillWidth: true
                        Layout.bottomMargin: 16
                        spacing: 16
                        Frame {
                            Layout.fillWidth: true
                            padding: 8
                            background: Rectangle {
                                color: Theme.backgroundPrimaryColor
                                border.width: 1
                                border.color: Theme.borderColor
                                radius: 4
                            }
                            RowLayout {
                                spacing: 4
                                anchors.fill: parent
                                ColumnLayout {
                                    spacing: 4
                                    Layout.fillWidth: true
                                    Label {
                                        text: qsTr("Path")
                                        font.pixelSize: 14
                                        font.weight: Font.DemiBold
                                    }
                                    SelectableLabel {
                                        Layout.fillWidth: true
                                        ThemedItem.foregroundLevel: SVS.FL_Secondary
                                        text: d.fileLocker?.path || qsTr("Unspecified")
                                        wrapMode: Text.Wrap
                                    }
                                }
                                ToolButton {
                                    icon.source: "image://fluent-system-icons/open"
                                    display: AbstractButton.IconOnly
                                    text: qsTr("Reveal in %1").arg(DesktopServices.fileManagerName)
                                    enabled: Boolean(d.fileLocker?.path)
                                    onClicked: () => {
                                        DesktopServices.reveal(d.fileLocker.path)
                                    }
                                }
                            }

                        }
                        InfoCard {
                            title: qsTr("Title")
                            text: ""
                        }
                        InfoCard {
                            title: qsTr("Author")
                            text: ""
                        }
                        InfoCard {
                            title: qsTr("Cent Shift")
                            text: ""
                        }
                    }
                }
            }
        }
    }
}