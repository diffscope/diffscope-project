import QtQml
import QtQml.Models
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
    title: qsTr("Phoneme")

    ColumnLayout {
        width: parent.width

        component PhonemeEditor: Frame {
            id: editor
            required property DspxModel.PhonemeSequence phonemeSequence
            property bool readOnly: false
            property int transactionId: 0

            padding: 4

            function beginTransaction() {
                let a = groupBox.windowHandle.projectDocumentContext.document.transactionController.beginTransaction()
                if (a) {
                    transactionId = a
                    return true
                }
                return false
            }
            function commitTransaction() {
                groupBox.windowHandle.projectDocumentContext.document.transactionController.commitTransaction(transactionId, qsTr("Editing phoneme"))
                transactionId = 0
            }

            ColumnLayout {
                anchors.fill: parent
                RowLayout {
                    id: headerRow
                    uniformCellSizes: true
                    Layout.fillWidth: true
                    Label {
                        text: qsTr("Token")
                        Layout.fillWidth: true
                        horizontalAlignment: Qt.AlignHCenter
                    }
                    Label {
                        text: qsTr("Start (ms)")
                        Layout.fillWidth: true
                        horizontalAlignment: Qt.AlignHCenter
                    }
                    Label {
                        text: qsTr("Language")
                        Layout.fillWidth: true
                        horizontalAlignment: Qt.AlignHCenter
                    }
                    Label {
                        text: qsTr("Onset")
                        Layout.fillWidth: true
                        horizontalAlignment: Qt.AlignHCenter
                    }
                }
                Repeater {
                    model: PhonemeListModel {
                        phonemeSequence: editor.phonemeSequence
                    }
                    readonly property Component editableComponent: RowLayout {
                        id: phonemeRow
                        required property var model
                        Layout.fillWidth: true
                        uniformCellSizes: true
                        TextField {
                            text: phonemeRow.model.token
                            Layout.fillWidth: true
                            onEditingFinished: () => {
                                if (text === phonemeRow.model.token)
                                    return
                                if (!editor.beginTransaction())
                                    return
                                let editor_ = editor
                                if (text.length !== 0) {
                                    phonemeRow.model.token = text
                                } else {
                                    editor_.phonemeSequence.removeItem(phonemeRow.model.phoneme)
                                }
                                editor_.commitTransaction()
                            }
                        }
                        SpinBox {
                            from: -2147483648
                            to: 2147483647
                            value: phonemeRow.model.start
                            Layout.fillWidth: true
                            onValueChanged: () => {
                                if (value === phonemeRow.model.start)
                                    return
                                if (!editor.beginTransaction())
                                    return
                                phonemeRow.model.start = value
                                editor.commitTransaction()
                            }
                        }
                        TextField {
                            // TODO language selector
                            text: phonemeRow.model.language
                            Layout.fillWidth: true
                            onEditingFinished: () => {
                                if (text === phonemeRow.model.language)
                                    return
                                if (!editor.beginTransaction())
                                    return
                                phonemeRow.model.language = text
                                editor.commitTransaction()
                            }
                        }
                        CheckBox {
                            checked: phonemeRow.model.onset
                            Layout.fillWidth: true
                            onClicked: () => {
                                if (checked === phonemeRow.model.onset)
                                    return
                                if (!editor.beginTransaction())
                                    return
                                phonemeRow.model.onset = checked
                                editor.commitTransaction()
                            }
                        }
                    }
                    readonly property Component readonlyComponent: RowLayout {
                        id: phonemeRow
                        required property var model
                        Layout.fillWidth: true
                        uniformCellSizes: true
                        TextEdit {
                            text: phonemeRow.model.token
                            Layout.fillWidth: true
                            font: Theme.font
                            color: Theme.foregroundPrimaryColor
                            horizontalAlignment: Qt.AlignHCenter
                            readOnly: true
                            selectionColor: Theme.accentColor
                        }
                        TextEdit {
                            text: phonemeRow.model.start.toLocaleString()
                            Layout.fillWidth: true
                            font: Theme.font
                            color: Theme.foregroundPrimaryColor
                            horizontalAlignment: Qt.AlignHCenter
                            readOnly: true
                            selectionColor: Theme.accentColor
                        }
                        TextEdit {
                            text: phonemeRow.model.language
                            Layout.fillWidth: true
                            font: Theme.font
                            color: Theme.foregroundPrimaryColor
                            horizontalAlignment: Qt.AlignHCenter
                            readOnly: true
                            selectionColor: Theme.accentColor
                        }
                        TextEdit {
                            text: phonemeRow.model.onset ? qsTr("Yes") : qsTr("No")
                            Layout.fillWidth: true
                            font: Theme.font
                            color: Theme.foregroundPrimaryColor
                            horizontalAlignment: Qt.AlignHCenter
                            readOnly: true
                            selectionColor: Theme.accentColor
                        }
                    }
                    delegate: editor.readOnly ? readonlyComponent : editableComponent
                }
            }
        }

        FormGroup {
            Layout.fillWidth: true
            label: qsTr("Original")
            columnItem: PhonemeEditor {
                enabled: groupBox.propertyMapper?.phonemes !== undefined
                readOnly: true
                phonemeSequence: groupBox.propertyMapper?.phonemes?.original ?? null
            }
        }

        FormGroup {
            Layout.fillWidth: true
            label: qsTr("Edited")
            InsertItemScenario {
                id: insertItemScenario
                window: groupBox.windowHandle?.window ?? null
                projectTimeline: groupBox.windowHandle?.projectTimeline ?? null
                document: groupBox.windowHandle?.projectDocumentContext.document ?? null
            }
            rowItem: ToolButton {
                enabled: groupBox.propertyMapper?.phonemes !== undefined
                text: qsTr("Insert")
                icon.source: "image://fluent-system-icons/add_circle"
                display: AbstractButton.IconOnly
                onClicked: insertItemScenario.insertPhoneme()
            }
            columnItem: PhonemeEditor {
                enabled: groupBox.propertyMapper?.phonemes !== undefined
                phonemeSequence: groupBox.propertyMapper?.phonemes?.edited ?? null
            }
        }
    }
}