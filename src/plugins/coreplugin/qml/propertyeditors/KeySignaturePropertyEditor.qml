import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

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
        FormGroup {
            Layout.fillWidth: true
            label: qsTr("Position")
            columnItem: TextField {
                text: groupBox.propertyMapper?.pos !== undefined ? GlobalHelper.musicTimelineTextFromValue(groupBox.windowHandle?.projectTimeline.musicTimeline ?? null, groupBox.propertyMapper.pos, 1, 1, 3) : ""
                readOnly: true
                ThemedItem.flat: true
            }
        }
        AbstractPropertyEditorField {
            id: pianoField
            windowHandle: groupBox.windowHandle
            propertyMapper: groupBox.propertyMapper
            key: ""
            transactionName: qsTr("Editing key signature")
            MusicModeMaskSelector {
                Layout.fillWidth: true
                mode: groupBox.propertyMapper?.mode !== undefined ? groupBox.propertyMapper.mode : 0
                tonality: groupBox.propertyMapper?.tonality !== undefined ? groupBox.propertyMapper.tonality : -1
                onModified: () => {
                    if (mode === groupBox.propertyMapper?.mode && tonality === groupBox.propertyMapper?.tonality)
                        return
                    pianoField.beginTransaction()
                    if (!pianoField.transactionId)
                        return
                    pianoField.propertyMapper.mode = mode
                    pianoField.propertyMapper.tonality = tonality
                    pianoField.commitTransaction()
                }
            }
        }
        AbstractPropertyEditorField {
            id: tonalityField
            windowHandle: groupBox.windowHandle
            propertyMapper: groupBox.propertyMapper
            key: "tonality"
            label: qsTr("Tonality")
            transactionName: qsTr("Editing key signature tonality")
            FormGroup {
                Layout.fillWidth: true
                label: tonalityField.label
                columnItem: ComboBox {
                    id: tonalityComboBox
                    model: ["C", "C\u266f/D\u266d", "D", "D\u266f/E\u266d", "E", "F", "F\u266f/G\u266d", "G", "G\u266f/A\u266d", "A", "A\u266f/B\u266d", "B"]
                    enabled: groupBox.propertyMapper?.mode !== 0
                    currentIndex: tonalityField.value !== undefined ? tonalityField.value : -1
                    onActivated: (index) => tonalityField.setValue(index)
                }
            }
        }
        AbstractPropertyEditorField {
            id: modeField
            windowHandle: groupBox.windowHandle
            propertyMapper: groupBox.propertyMapper
            key: "mode"
            label: qsTr("Mode")
            transactionName: qsTr("Editing key signature mode")
            FormGroup {
                Layout.fillWidth: true
                label: modeField.label
                columnItem: ComboBox {
                    id: modeComboBox
                    textRole: "name"
                    valueRole: "musicMode"
                    model: SVS.getBuiltInMusicModeInfoList()
                    displayText: modeField.value === undefined ? "" : currentText.length ? currentText : qsTr("Custom Mode")
                    currentValue: modeField.value
                    onActivated: (index) => modeField.setValue(valueAt(index))
                }
            }
        }
        AbstractPropertyEditorField {
            id: accidentalTypeField
            windowHandle: groupBox.windowHandle
            propertyMapper: groupBox.propertyMapper
            key: "accidentalType"
            label: qsTr("Accidental type")
            transactionName: qsTr("Editing key signature accidental type")
            FormGroup {
                Layout.fillWidth: true
                label: accidentalTypeField.label
                columnItem: Item {
                    implicitHeight: rowLayout.implicitHeight
                    Rectangle {
                        anchors.fill: parent
                        color: Theme.buttonColor
                        radius: 4
                    }
                    RowLayout {
                        id: rowLayout
                        spacing: 0
                        anchors.fill: parent
                        ToolButton {
                            Layout.fillWidth: true
                            ThemedItem.controlType: SVS.CT_Accent
                            icon.source: "qrc:/diffscope/coreplugin/icons/accidental_flat.svg"
                            text: qsTr("Flat")
                            checkable: true
                            checked: accidentalTypeField.value === 0
                            onClicked: accidentalTypeField.setValue(0)
                        }
                        ToolButton {
                            Layout.fillWidth: true
                            ThemedItem.controlType: SVS.CT_Accent
                            icon.source: "qrc:/diffscope/coreplugin/icons/accidental_sharp.svg"
                            text: qsTr("Sharp")
                            checkable: true
                            checked: accidentalTypeField.value === 1
                            onClicked: accidentalTypeField.setValue(1)
                        }
                    }
                    Rectangle {
                        anchors.fill: parent
                        color: "transparent"
                        radius: 4
                        border.width: 1
                        border.color: Theme.borderColor
                    }
                }
            }
        }
    }
}