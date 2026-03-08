import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Templates as T
import QtQuick.Controls.impl

import SVSCraft
import SVSCraft.UIComponents
import SVSCraft.UIComponents.impl

import DiffScope.UIShell

QtObject {
    id: d

    required property QtObject addOn

    readonly property Component importPanelComponent: ActionDockingPane {
        id: pane
        ThemedItem.backgroundLevel: addOn?.isHomeWindow ? SVS.BL_Quaternary : SVS.BL_Primary
        ColumnLayout {
            anchors.fill: parent
            anchors.topMargin: addOn?.isHomeWindow ? 0 : 16
            spacing: 16
            Label {
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                font.pixelSize: 16
                text: qsTr("Select a format to import")
            }
            ScrollView {
                id: scrollView
                Layout.fillWidth: true
                Layout.fillHeight: true
                ColumnLayout {
                    width: scrollView.width - 32
                    x: 16
                    spacing: 16
                    Repeater {
                        model: DelegateModel {
                            model: d.addOn?.importConverters ?? []
                            delegate: T.Button {
                                id: control
                                required property QtObject modelData
                                Layout.fillWidth: true
                                implicitHeight: implicitContentHeight + topPadding + bottomPadding
                                padding: 8
                                rightPadding: 24
                                font.pixelSize: 16
                                text: modelData.name
                                onClicked: d.addOn.execImport(modelData)
                                background: ButtonRectangle {
                                    control: control
                                    IconImage {
                                        anchors.right: parent.right
                                        anchors.rightMargin: 8
                                        anchors.verticalCenter: parent.verticalCenter
                                        source: "image://fluent-system-icons/chevron_right"
                                        sourceSize.width: 16
                                        sourceSize.height: 16
                                        color: Theme.foregroundPrimaryColor
                                    }
                                }
                                contentItem: ColumnLayout {
                                    Text {
                                        Layout.fillWidth: true
                                        font: control.font
                                        text: control.text
                                        wrapMode: Text.Wrap
                                        color: Theme.foregroundPrimaryColor
                                    }
                                    Text {
                                        Layout.fillWidth: true
                                        font: Theme.font
                                        text: modelData.description
                                        wrapMode: Text.Wrap
                                        color: Theme.foregroundSecondaryColor
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

    }
}