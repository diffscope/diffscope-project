import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SVSCraft
import SVSCraft.UIComponents

import DiffScope.UIShell

GroupBox {
    id: groupBox
    required property ProjectWindowInterface windowHandle
    required property QtObject propertyMapper
    title: qsTr("Control")
    ThemedItem.foldable: true
    Item {
        width: parent.width
        visible: opacity !== 0
        opacity: groupBox.ThemedItem.folded ? 0 : 1
        implicitHeight: groupBox.ThemedItem.folded ? 0 : columnLayout.implicitHeight
        clip: true
        Behavior on opacity {
            NumberAnimation {
                duration: Theme.visualEffectAnimationDuration
                easing.type: Easing.OutCubic
            }
        }
        Behavior on implicitHeight {
            NumberAnimation {
                duration: Theme.visualEffectAnimationDuration
                easing.type: Easing.OutCubic
            }
        }
        ColumnLayout {
            id: columnLayout
            anchors.fill: parent
            CheckBox {
                text: qsTr("Mute")
                tristate: true
            }
            CheckBox {
                text: qsTr("Solo")
                tristate: true
            }
            CheckBox {
                text: qsTr("Record")
                tristate: true
            }
            FormGroup {
                Layout.fillWidth: true
                label: qsTr("Gain")
                rowItem: SpinBox {

                }
                columnItem: Slider {

                }
            }
            FormGroup {
                Layout.fillWidth: true
                label: qsTr("Pan")
                rowItem: SpinBox {

                }
                columnItem: Slider {

                }
            }
        }
    }
}