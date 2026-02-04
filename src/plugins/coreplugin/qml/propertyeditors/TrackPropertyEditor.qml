import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Templates as T

import SVSCraft
import SVSCraft.UIComponents

import DiffScope.UIShell

PropertyEditorGroupBox {
    id: groupBox
    required property ProjectWindowInterface windowHandle
    required property QtObject propertyMapper
    title: qsTr("Basic")
    ColumnLayout {
        width: parent.width
        TextPropertyEditorField {
            windowHandle: groupBox.windowHandle
            propertyMapper: groupBox.propertyMapper
            key: "name"
            label: qsTr("Name")
            transactionName: qsTr("Renaming track")
        }
        FormGroup {
            id: colorControl
            Layout.fillWidth: true
            label: qsTr("Color")
            readonly property var colorId: groupBox.propertyMapper?.colorId
            property int transactionId: 0
            function beginTransaction() {
                let a = groupBox.windowHandle.projectDocumentContext.document.transactionController.beginTransaction()
                if (a) {
                    transactionId = a
                    return true
                }
                return false
            }
            function commitTransaction() {
                groupBox.windowHandle.projectDocumentContext.document.transactionController.commitTransaction(transactionId, "Picking track color")
                transactionId = 0
            }
            columnItem: Flow {
                spacing: 16
                topPadding: 8
                Repeater {
                    model: CoreInterface.trackColorSchema.colors
                    delegate: T.Button {
                        id: control
                        implicitWidth: 16
                        implicitHeight: 16
                        checkable: true
                        checked: index === colorControl.colorId
                        autoExclusive: true
                        required property color modelData
                        required property int index
                        background: Rectangle {
                            color: modelData
                            border.color: control.checked ? Theme.foregroundPrimaryColor : Theme.borderColor
                            border.width: control.checked ? 2 : 1
                        }
                        text: qsTr("Track Color %L1").arg(index + 1)
                        onClicked: {
                            if (colorControl.colorId === index) {
                                return
                            }
                            colorControl.beginTransaction()
                            if (!colorControl.transactionId)
                                return
                            groupBox.propertyMapper.colorId = index
                            colorControl.commitTransaction()
                        }
                    }
                }
            }
        }
    }
}