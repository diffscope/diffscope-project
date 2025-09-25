import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Templates as T
import QtQuick.Controls.impl
import QtQuick.Layouts
import QtQml.Models

import SVSCraft
import SVSCraft.UIComponents
import SVSCraft.UIComponents.impl

MessageBoxDialog {
    id: dialog
    width: 400
    icon: SVS.Warning
    hasContextHelp: true
    text: qsTr("%1 exited abnormally during the last initialization").arg(Application.name)
    informativeText: qsTr('This is probably caused by a faulty plugin. Please <a href="#logs">check the logs</a> for more information.<br/><br/>You may view the plugin list and disable some plugins.')

    required property string logsPath

    readonly property PluginManagerHelper helper: PluginManagerHelper {
        id: helper
    }

    onLinkActivated: () => {
        DesktopServices.reveal(logsPath)
    }

    content: Frame {
        implicitHeight: 300
        ListView {
            anchors.fill: parent
            anchors.margins: 1
            clip: true
            model: {
                let a = []
                helper.pluginCollections.forEach(o => {
                    a.push(...o.plugins)
                })
                return a
            }
            delegate: ItemDelegate {
                id: control
                required property var model
                ThemedItem.flat: true
                padding: 4
                leftPadding: 8
                rightPadding: 8
                width: ListView.view.width
                background: ButtonRectangle {
                    implicitWidth: 100
                    implicitHeight: 24
                    control: control
                    checked: false
                    flat: true
                }
                contentItem: RowLayout {
                    ColumnLayout {
                        Layout.alignment: Qt.AlignVCenter
                        spacing: 4
                        RowLayout {
                            spacing: 16
                            Label {
                                text: control.model.displayName
                                font.weight: Font.DemiBold
                            }
                            Label {
                                text: control.model.name
                                ThemedItem.foregroundLevel: SVS.FL_Secondary
                            }
                        }
                        Label {
                            Layout.fillWidth: true
                            ThemedItem.foregroundLevel: SVS.FL_Secondary
                            wrapMode: Text.WrapAnywhere
                            text: control.model.filePath
                        }
                    }
                    Item {
                        Layout.fillWidth: true
                    }
                    Switch {
                        Layout.alignment: Qt.AlignVCenter
                        enabled: !control.model.required
                        checked: control.model.enabledBySettings
                        onClicked: control.model.enabledBySettings = checked
                    }
                }
                onDoubleClicked: () => {
                    DesktopServices.reveal(control.model.filePath)
                }
            }
        }
    }
}