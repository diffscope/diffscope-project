import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SVSCraft
import SVSCraft.UIComponents

import DiffScope.UIShell

PropertyEditorGroupBox {
    id: groupBox
    required property ProjectWindowInterface windowHandle
    required property QtObject propertyMapper
    title: qsTr("Control")
    ColumnLayout {
        id: columnLayout
        width: parent.width
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