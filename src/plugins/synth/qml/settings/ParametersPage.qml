import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl
import QtQuick.Dialogs
import QtQuick.Layouts

import SVSCraft
import SVSCraft.UIComponents
import SVSCraft.UIComponents.impl

Item {
    id: page

    required property QtObject pageHandle
    required property var configurationModel
    property bool started: false
    readonly property TextMatcher matcher: TextMatcher {}
    readonly property var currentParameter: parameterList.currentItem?.parameterModel ?? null

    anchors.fill: parent

    FileDialog {
        id: importDialog
        title: qsTr("Import Parameter Configurations")
        fileMode: FileDialog.OpenFile
        nameFilters: [qsTr("JSON files (*.json)"), qsTr("All files (*)")]
        onAccepted: page.pageHandle.importFromFile(selectedFile)
    }

    FileDialog {
        id: exportDialog
        title: qsTr("Export Parameter Configurations")
        fileMode: FileDialog.SaveFile
        defaultSuffix: "json"
        nameFilters: [qsTr("JSON files (*.json)"), qsTr("All files (*)")]
        onAccepted: page.pageHandle.exportToFile(selectedFile)
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Label {
            Layout.fillWidth: true
            Layout.leftMargin: 12
            Layout.rightMargin: 12
            Layout.bottomMargin: visible ? 8 : 0
            visible: text.length > 0
            text: page.pageHandle.errorMessage
            color: Theme.errorColor
            wrapMode: Text.Wrap
        }
        Label {
            Layout.fillWidth: true
            Layout.leftMargin: 12
            Layout.rightMargin: 12
            Layout.bottomMargin: visible ? 8 : 0
            visible: text.length > 0
            text: page.pageHandle.statusMessage
            color: Theme.accentColor
            wrapMode: Text.Wrap
        }

        SplitView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.margins: 12

            Frame {
                SplitView.preferredWidth: 220
                SplitView.minimumWidth: 220
                SplitView.maximumWidth: 400
                padding: 1

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 0

                    ListView {
                        id: parameterList
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        focus: true
                        model: page.configurationModel
                        currentIndex: -1

                        onCountChanged: {
                            if (count === 0)
                                currentIndex = -1
                            else if (currentIndex < 0)
                                currentIndex = 0
                            else if (currentIndex >= count)
                                currentIndex = count - 1
                        }

                        delegate: ItemDelegate {
                            id: parameterDelegate
                            required property var model
                            required property int index
                            property var parameterModel: model

                            width: ListView.view.width
                            padding: 4
                            leftPadding: 8
                            rightPadding: 8
                            highlighted: ListView.isCurrentItem
                            ThemedItem.flat: true
                            ThemedItem.controlType: highlighted ? SVS.CT_Accent : SVS.CT_Normal
                            background: ButtonRectangle {
                                control: parameterDelegate
                                checked: parameterDelegate.highlighted
                                flat: true
                            }
                            contentItem: RowLayout {
                                spacing: 8
                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 1
                                    Label {
                                        Layout.fillWidth: true
                                        text: parameterDelegate.model.displayName
                                        font.weight: Font.DemiBold
                                        elide: Text.ElideRight
                                    }
                                    Label {
                                        Layout.fillWidth: true
                                        text: parameterDelegate.model.architectureId
                                              + " / " + parameterDelegate.model.parameterId
                                        elide: Text.ElideMiddle
                                        ThemedItem.foregroundLevel: SVS.FL_Secondary
                                    }
                                }
                                IconLabel {
                                    visible: parameterDelegate.model.builtin
                                    icon.source: "image://fluent-system-icons/lock_closed"
                                    icon.width: 16
                                    icon.height: 16
                                    icon.color: Theme.foregroundSecondaryColor
                                }
                            }
                            onClicked: parameterList.currentIndex = index
                        }

                        Label {
                            anchors.centerIn: parent
                            visible: parameterList.count === 0
                            text: qsTr("No parameter configurations")
                            ThemedItem.foregroundLevel: SVS.FL_Secondary
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        implicitHeight: 1
                        color: Theme.borderColor
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        Layout.margins: 4
                        spacing: 2

                        ToolButton {
                            text: qsTr("Add Parameter")
                            display: AbstractButton.IconOnly
                            icon.source: "image://fluent-system-icons/add"
                            onClicked: () => parameterList.currentIndex = page.configurationModel.addParameter()
                        }
                        ToolButton {
                            text: qsTr("Delete")
                            display: AbstractButton.IconOnly
                            icon.source: "image://fluent-system-icons/delete"
                            enabled: Boolean(page.currentParameter && !page.currentParameter.builtin)
                            onClicked: () => {
                                const oldIndex = parameterList.currentIndex
                                if (page.configurationModel.removeParameter(oldIndex))
                                    parameterList.currentIndex = Math.min(oldIndex, parameterList.count - 1)
                            }
                        }
                        ToolButton {
                            text: qsTr("Import")
                            display: AbstractButton.IconOnly
                            icon.source: "image://fluent-system-icons/document_arrow_right"
                            onClicked: importDialog.open()
                        }
                        ToolButton {
                            text: qsTr("Export All")
                            display: AbstractButton.IconOnly
                            icon.source: "image://fluent-system-icons/document_arrow_left"
                            onClicked: exportDialog.open()
                        }
                        Item { Layout.fillWidth: true }
                    }
                }
            }

            ScrollView {
                SplitView.fillWidth: true
                SplitView.fillHeight: true
                contentWidth: availableWidth

                ColumnLayout {
                    width: parent.width
                    spacing: 12

                    GridLayout {
                        Layout.fillWidth: true
                        Layout.leftMargin: 12
                        Layout.rightMargin: 12
                        columns: 2
                        columnSpacing: 12
                        rowSpacing: 8
                        enabled: Boolean(page.currentParameter && !page.currentParameter.builtin)

                        Label { text: qsTr("Architecture ID") }
                        TextField {
                            Layout.fillWidth: true
                            text: page.currentParameter?.architectureId ?? ""
                            onTextEdited: page.currentParameter.architectureId = text
                        }
                        Label { text: qsTr("Parameter ID") }
                        TextField {
                            Layout.fillWidth: true
                            text: page.currentParameter?.parameterId ?? ""
                            onTextEdited: page.currentParameter.parameterId = text
                        }
                        Label { text: qsTr("Display name") }
                        TextField {
                            Layout.fillWidth: true
                            text: page.currentParameter?.displayName ?? ""
                            onTextEdited: page.currentParameter.displayName = text
                        }
                        Label { text: qsTr("Minimum value") }
                        SpinBox {
                            Layout.fillWidth: true
                            from: -2147483647
                            to: 2147483647
                            editable: true
                            value: page.currentParameter?.minimumValue ?? 0
                            onValueModified: page.currentParameter.minimumValue = value
                        }
                        Label { text: qsTr("Maximum value") }
                        SpinBox {
                            Layout.fillWidth: true
                            from: -2147483647
                            to: 2147483647
                            editable: true
                            value: page.currentParameter?.maximumValue ?? 1000
                            onValueModified: page.currentParameter.maximumValue = value
                        }
                        CheckBox {
                            Layout.columnSpan: 2
                            text: qsTr("Has default value")
                            checked: page.currentParameter?.showDefaultValue ?? false
                            onClicked: page.currentParameter.showDefaultValue = checked
                        }
                        Label {
                            text: qsTr("Default value")
                            enabled: page.currentParameter?.showDefaultValue ?? false
                        }
                        SpinBox {
                            Layout.fillWidth: true
                            enabled: page.currentParameter?.showDefaultValue ?? false
                            from: -2147483647
                            to: 2147483647
                            editable: true
                            value: page.currentParameter?.defaultValue ?? 0
                            onValueModified: page.currentParameter.defaultValue = value
                        }
                        Label { text: qsTr("Fill mode") }
                        ComboBox {
                            id: fillModeComboBox
                            Layout.fillWidth: true
                            textRole: "text"
                            valueRole: "value"
                            model: [
                                { text: qsTr("No fill"), value: 0 },
                                { text: qsTr("Top fill"), value: 1 },
                                { text: qsTr("Bottom fill"), value: 2 },
                                { text: qsTr("Baseline fill"), value: 3 }
                            ]
                            currentIndex: page.currentParameter ? indexOfValue(page.currentParameter.fillMode) : -1
                            onActivated: page.currentParameter.fillMode = currentValue
                        }
                        Label { text: qsTr("Value type") }
                        ComboBox {
                            id: valueTypeComboBox
                            Layout.fillWidth: true
                            textRole: "text"
                            valueRole: "value"
                            model: [
                                { text: qsTr("Absolute"), value: 0 },
                                { text: qsTr("Relative"), value: 1 }
                            ]
                            currentIndex: page.currentParameter ? indexOfValue(page.currentParameter.valueType) : -1
                            onActivated: page.currentParameter.valueType = currentValue
                        }
                        CheckBox {
                            Layout.columnSpan: 2
                            text: qsTr("Show divisions")
                            checked: page.currentParameter?.showDivision ?? false
                            onClicked: page.currentParameter.showDivision = checked
                        }
                        Label {
                            text: qsTr("Division interval")
                            enabled: page.currentParameter?.showDivision ?? false
                        }
                        SpinBox {
                            Layout.fillWidth: true
                            enabled: page.currentParameter?.showDivision ?? false
                            from: 1
                            to: 2147483647
                            editable: true
                            value: page.currentParameter?.divisionValue ?? 1
                            onValueModified: page.currentParameter.divisionValue = value
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        Layout.leftMargin: 12
                        Layout.rightMargin: 12
                        enabled: Boolean(page.currentParameter && !page.currentParameter.builtin)
                        spacing: 8

                        Label {
                            Layout.fillWidth: true
                            text: qsTr("Value Mapping Expressions")
                            font.weight: Font.DemiBold
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            implicitHeight: 1
                            color: Theme.borderColor
                        }

                        GridLayout {
                            Layout.fillWidth: true
                            columns: 2
                            columnSpacing: 12
                            rowSpacing: 8
                            Label { text: qsTr("Normalization") }
                            TextField {
                                Layout.fillWidth: true
                                text: page.currentParameter?.normalizationExpression ?? ""
                                onTextEdited: page.currentParameter.normalizationExpression = text
                            }
                            Label { text: qsTr("Inverse normalization") }
                            TextField {
                                Layout.fillWidth: true
                                text: page.currentParameter?.denormalizationExpression ?? ""
                                onTextEdited: page.currentParameter.denormalizationExpression = text
                            }
                            Label { text: qsTr("Display value mapping") }
                            TextField {
                                Layout.fillWidth: true
                                text: page.currentParameter?.displayValueExpression ?? ""
                                onTextEdited: page.currentParameter.displayValueExpression = text
                            }
                            Label { text: qsTr("Inverse display value mapping") }
                            TextField {
                                Layout.fillWidth: true
                                text: page.currentParameter?.displayValueInverseExpression ?? ""
                                onTextEdited: page.currentParameter.displayValueInverseExpression = text
                            }
                            Label { text: qsTr("Display text template") }
                            TextField {
                                Layout.fillWidth: true
                                text: page.currentParameter?.displayTextTemplate ?? ""
                                placeholderText: qsTr("%d, %.2f, %.3f and %% are supported")
                                onTextEdited: page.currentParameter.displayTextTemplate = text
                            }
                        }
                    }

                    Label {
                        Layout.fillWidth: true
                        Layout.margins: 12
                        visible: !page.currentParameter
                        horizontalAlignment: Text.AlignHCenter
                        text: qsTr("Select a parameter to edit it.")
                        ThemedItem.foregroundLevel: SVS.FL_Secondary
                    }
                    Item { Layout.fillHeight: true }
                }
            }
        }
    }
}
