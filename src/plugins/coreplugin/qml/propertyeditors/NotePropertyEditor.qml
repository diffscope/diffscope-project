import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SVSCraft
import SVSCraft.UIComponents

import DiffScope.UIShell
import DiffScope.DspxModel as DspxModel

PropertyEditorGroupBox {
    id: groupBox
    required property ProjectWindowInterface windowHandle
    required property QtObject propertyMapper
    title: qsTr("Basic")

    ColumnLayout {
        width: parent.width

        FormGroup {
            Layout.fillWidth: true
            label: qsTr("Associated singing clip")
            rowItem: ToolButton {
                text: qsTr("Select singing clip")
                flat: true
                display: AbstractButton.IconOnly
                icon.source: "image://fluent-system-icons/location_target_square"
                onClicked: () => {
                    if (!groupBox.propertyMapper?.singingClip)
                        return
                    let selectionModel = groupBox.windowHandle?.projectDocumentContext.document.selectionModel
                    if (!selectionModel)
                        return
                    selectionModel.select(groupBox.propertyMapper.singingClip, DspxModel.SelectionModel.Select | DspxModel.SelectionModel.SetCurrentItem)
                }
            }
            columnItem: TextField {
                text: groupBox.propertyMapper?.singingClip === undefined
                      ? qsTr("Multiple clips")
                      : groupBox.propertyMapper?.singingClip ? groupBox.propertyMapper.singingClip.name : qsTr("None")
                readOnly: true
                ThemedItem.flat: true
            }
        }
    }
}