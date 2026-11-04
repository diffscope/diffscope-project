// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Templates as T

import SVSCraft
import SVSCraft.UIComponents
import SVSCraft.UIComponents.impl

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
        AbstractPropertyEditorField {
            id: colorField
            windowHandle: groupBox.windowHandle
            propertyMapper: groupBox.propertyMapper
            key: "colorId"
            label: qsTr("Color")
            transactionName: qsTr("Picking track color")
            FormGroup {
                Layout.fillWidth: true
                label: colorField.label
                columnItem: Flow {
                    spacing: 16
                    topPadding: 8
                    Repeater {
                        id: colorButtons
                        model: CoreInterface.trackColorSchema.colors
                        delegate: T.Button {
                            id: control
                            implicitWidth: 16
                            implicitHeight: 16
                            checkable: true
                            checked: index === colorField.value
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
                                if (colorField.setValue(index))
                                    return
                                GlobalHelper.setProperty(control, "checked", false)
                                const previousButton = colorButtons.itemAt(colorField.value ?? -1)
                                if (previousButton)
                                    GlobalHelper.setProperty(previousButton, "checked", true)
                            }
                        }
                    }
                }
            }
        }
    }
}