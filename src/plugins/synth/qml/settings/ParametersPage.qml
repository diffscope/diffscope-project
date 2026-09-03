// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

import QtQml
import QtQuick
import QtQuick.Controls
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

                    GroupBox {
                        Layout.fillWidth: true
                        Layout.leftMargin: 12
                        Layout.rightMargin: 12
                        enabled: Boolean(page.currentParameter && !page.currentParameter.builtin)
                        title: qsTr("Parameter Configuration")
                        TextMatcherItem on title {
                            matcher: page.matcher
                        }

                        GridLayout {
                            anchors.fill: parent
                            columns: 2
                            columnSpacing: 12
                            rowSpacing: 8

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
                            CheckBox {
                                Layout.columnSpan: 2
                                text: qsTr("Show default value")
                                checked: page.currentParameter?.showDefaultValue ?? false
                                onClicked: page.currentParameter.showDefaultValue = checked
                            }
                            Label {
                                text: qsTr("Default value")
                            }
                            DoubleSpinBox {
                                Layout.fillWidth: true
                                from: 0.0
                                to: 1.0
                                decimals: 3
                                stepSize: 0.001
                                editable: true
                                value: page.currentParameter?.defaultValue ?? 0.0
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
                            DoubleSpinBox {
                                Layout.fillWidth: true
                                enabled: page.currentParameter?.showDivision ?? false
                                from: 0.001
                                to: 1.0
                                decimals: 3
                                stepSize: 0.001
                                editable: true
                                value: page.currentParameter?.divisionValue ?? 0.2
                                onValueModified: page.currentParameter.divisionValue = value
                            }
                        }
                    }

                    GroupBox {
                        Layout.fillWidth: true
                        Layout.leftMargin: 12
                        Layout.rightMargin: 12
                        enabled: Boolean(page.currentParameter && !page.currentParameter.builtin)
                        title: qsTr("Display Mapping Expressions")
                        TextMatcherItem on title {
                            matcher: page.matcher
                        }

                        GridLayout {
                            anchors.fill: parent
                            columns: 2
                            columnSpacing: 12
                            rowSpacing: 8
                            Label {
                                id: displayValueMappingLabel
                                readonly property string description: qsTr("Maps a normalized parameter value to its displayed value.")
                                text: qsTr("Display value mapping")
                                DescriptiveText.toolTip: description
                                DescriptiveText.activated: displayValueMappingHoverHandler.hovered
                                HoverHandler { id: displayValueMappingHoverHandler }
                            }
                            TextField {
                                Layout.fillWidth: true
                                text: page.currentParameter?.displayValueExpression ?? ""
                                onTextEdited: page.currentParameter.displayValueExpression = text
                                Accessible.description: displayValueMappingLabel.description
                            }
                            Label {
                                id: inverseDisplayValueMappingLabel
                                readonly property string description: qsTr("Maps a displayed parameter value back to its normalized value.")
                                text: qsTr("Inverse display value mapping")
                                DescriptiveText.toolTip: description
                                DescriptiveText.activated: inverseDisplayValueMappingHoverHandler.hovered
                                HoverHandler { id: inverseDisplayValueMappingHoverHandler }
                            }
                            TextField {
                                Layout.fillWidth: true
                                text: page.currentParameter?.displayValueInverseExpression ?? ""
                                onTextEdited: page.currentParameter.displayValueInverseExpression = text
                                Accessible.description: inverseDisplayValueMappingLabel.description
                            }
                            Label {
                                id: displayTextTemplateLabel
                                readonly property string description: qsTr("Formats the displayed parameter value. Use %d for a rounded integer, %.Nf for a fixed-point value with N decimal places, and %% for a literal percent sign.")
                                text: qsTr("Display text template")
                                DescriptiveText.toolTip: description
                                DescriptiveText.activated: displayTextTemplateHoverHandler.hovered
                                HoverHandler { id: displayTextTemplateHoverHandler }
                            }
                            TextField {
                                Layout.fillWidth: true
                                text: page.currentParameter?.displayTextTemplate ?? ""
                                placeholderText: qsTr("Use %d for integers, %.Nf for N decimal places, and %% for a percent sign")
                                onTextEdited: page.currentParameter.displayTextTemplate = text
                                Accessible.description: displayTextTemplateLabel.description
                            }
                        }
                    }

                    Label {
                        Layout.fillWidth: true
                        Layout.margins: 12
                        visible: !page.currentParameter
                        horizontalAlignment: Text.AlignHCenter
                        text: qsTr("Select a parameter to edit.")
                        ThemedItem.foregroundLevel: SVS.FL_Secondary
                    }
                    Item { Layout.fillHeight: true }
                }
            }
        }
    }
}
