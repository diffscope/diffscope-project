// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SVSCraft
import SVSCraft.UIComponents

Dialog {
    id: dialog

    property string descriptionText
    property var fieldDefinitions: []
    property string acceptText
    property string cancelText
    property var resultValues: []

    width: 560
    height: Math.max(320, Math.min(640, parent ? parent.height - 24 : 640))
    modal: true
    closePolicy: Popup.CloseOnEscape

    function submit() {
        let valid = true
        let firstInvalidEditor = null
        for (let index = 0; index < fieldRepeater.count; ++index) {
            const editor = fieldRepeater.itemAt(index).item
            if (!editor.validate(true)) {
                valid = false
                if (!firstInvalidEditor)
                    firstInvalidEditor = editor
            }
        }
        if (!valid) {
            firstInvalidEditor.focusEditor()
            return
        }

        const values = []
        for (let index = 0; index < fieldRepeater.count; ++index)
            values.push(fieldRepeater.itemAt(index).item.resultValue())
        resultValues = values
        accept()
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 12

        Label {
            Layout.fillWidth: true
            visible: text.length !== 0
            text: dialog.descriptionText
            wrapMode: Text.Wrap
        }

        ScrollView {
            id: formScrollView
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true

            ColumnLayout {
                width: formScrollView.availableWidth
                spacing: 16

                Repeater {
                    id: fieldRepeater
                    model: dialog.fieldDefinitions

                    delegate: Loader {
                        id: fieldLoader
                        required property var modelData

                        Layout.fillWidth: true
                        property var fieldData: modelData
                        sourceComponent: fieldData.type === "text" ? textEditor
                            : fieldData.type === "integer" || fieldData.type === "number" ? numberEditor
                            : fieldData.type === "boolean" ? booleanEditor
                            : fieldData.type === "choice" ? choiceEditor
                            : colorEditor
                        onLoaded: item.field = fieldData
                    }
                }
            }
        }
    }

    footer: DialogButtonBox {
        Button {
            text: dialog.acceptText
            DialogButtonBox.buttonRole: DialogButtonBox.ActionRole
            onClicked: dialog.submit()
        }
        Button {
            text: dialog.cancelText
            DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
            onClicked: dialog.reject()
        }
    }

    Component {
        id: textEditor

        ColumnLayout {
            id: textEditorRoot
            property var field
            property bool attempted

            readonly property string currentText: inputLoader.item ? inputLoader.item.editorText : ""
            readonly property bool valueRequired: field && field.required && currentText.trim().length === 0
            readonly property bool tooShort: field && currentText.length < field.minLength
            readonly property bool tooLong: field && field.maxLength >= 0 && currentText.length > field.maxLength
            readonly property bool patternMismatch: field && field.pattern.length !== 0 && !matchesPattern(currentText)
            readonly property bool valid: !valueRequired && !tooShort && !tooLong && !patternMismatch

            Layout.fillWidth: true
            spacing: 4

            function matchesPattern(value) {
                const match = new RegExp(`^(?:${field.pattern})$`).exec(value)
                return match !== null
            }

            function errorText() {
                if (valueRequired)
                    return qsTr("A value is required.")
                if (tooShort)
                    return qsTr("Enter at least %1 characters.").arg(Number(field.minLength).toLocaleString())
                if (tooLong)
                    return qsTr("Enter no more than %1 characters.").arg(Number(field.maxLength).toLocaleString())
                if (patternMismatch)
                    return qsTr("The value does not match the required format.")
                return ""
            }

            function validate(showError) {
                if (showError)
                    attempted = true
                return valid
            }

            function resultValue() {
                return currentText
            }

            function focusEditor() {
                inputLoader.item.focusInput()
            }

            Label {
                Layout.fillWidth: true
                text: textEditorRoot.field ? textEditorRoot.field.label : ""
                wrapMode: Text.Wrap
            }

            Loader {
                id: inputLoader
                Layout.fillWidth: true
                sourceComponent: textEditorRoot.field && textEditorRoot.field.multiline ? multilineTextInput : singleLineTextInput
            }

            Label {
                Layout.fillWidth: true
                visible: text.length !== 0
                text: textEditorRoot.field ? textEditorRoot.field.description : ""
                wrapMode: Text.Wrap
                ThemedItem.foregroundLevel: SVS.FL_Secondary
            }

            Label {
                Layout.fillWidth: true
                visible: textEditorRoot.attempted && !textEditorRoot.valid
                color: Theme.errorColor
                text: textEditorRoot.errorText()
                wrapMode: Text.Wrap
            }

            Component {
                id: singleLineTextInput

                TextField {
                    readonly property string editorText: text

                    function focusInput() {
                        forceActiveFocus()
                    }

                    text: textEditorRoot.field ? textEditorRoot.field.value : ""
                    placeholderText: textEditorRoot.field ? textEditorRoot.field.placeholder : ""
                    ThemedItem.controlType: textEditorRoot.attempted && !textEditorRoot.valid ? SVS.CT_Error : SVS.CT_Normal
                    Keys.onReturnPressed: dialog.submit()
                }
            }

            Component {
                id: multilineTextInput

                ScrollView {
                    readonly property string editorText: textArea.text

                    function focusInput() {
                        textArea.forceActiveFocus()
                    }

                    implicitHeight: 120

                    TextArea {
                        id: textArea
                        text: textEditorRoot.field ? textEditorRoot.field.value : ""
                        placeholderText: textEditorRoot.field ? textEditorRoot.field.placeholder : ""
                        wrapMode: TextEdit.Wrap
                        ThemedItem.controlType: textEditorRoot.attempted && !textEditorRoot.valid ? SVS.CT_Error : SVS.CT_Normal
                    }
                }
            }
        }
    }

    Component {
        id: numberEditor

        ColumnLayout {
            id: numberEditorRoot
            property var field
            readonly property real initialValue: field ? field.value : 0
            property real currentValue: initialValue

            Layout.fillWidth: true
            spacing: 4

            function validate() {
                return true
            }

            function resultValue() {
                return field.type === "integer" ? Math.round(currentValue) : currentValue
            }

            function focusEditor() {
                spinBox.forceActiveFocus()
            }

            Label {
                Layout.fillWidth: true
                text: numberEditorRoot.field ? numberEditorRoot.field.label : ""
                wrapMode: Text.Wrap
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                Slider {
                    Layout.fillWidth: true
                    from: numberEditorRoot.field ? numberEditorRoot.field.minimum : 0
                    to: numberEditorRoot.field ? numberEditorRoot.field.maximum : 100
                    stepSize: numberEditorRoot.field ? numberEditorRoot.field.step : 1
                    value: numberEditorRoot.currentValue
                    onMoved: numberEditorRoot.currentValue = value
                    ThemedItem.doubleClickResetValue: numberEditorRoot.initialValue
                    ThemedItem.onDoubleClickReset: moved()
                }

                DoubleSpinBox {
                    id: spinBox
                    Layout.preferredWidth: 140
                    from: numberEditorRoot.field ? numberEditorRoot.field.minimum : 0
                    to: numberEditorRoot.field ? numberEditorRoot.field.maximum : 100
                    stepSize: numberEditorRoot.field ? numberEditorRoot.field.step : 1
                    decimals: numberEditorRoot.field ? numberEditorRoot.field.decimals : 0
                    value: numberEditorRoot.currentValue
                    editable: true
                    onValueModified: numberEditorRoot.currentValue = value
                }
            }

            Label {
                Layout.fillWidth: true
                visible: text.length !== 0
                text: numberEditorRoot.field ? numberEditorRoot.field.description : ""
                wrapMode: Text.Wrap
                ThemedItem.foregroundLevel: SVS.FL_Secondary
            }
        }
    }

    Component {
        id: booleanEditor

        ColumnLayout {
            id: booleanEditorRoot
            property var field

            Layout.fillWidth: true
            spacing: 4

            function validate() {
                return true
            }

            function resultValue() {
                return checkBox.checked
            }

            function focusEditor() {
                checkBox.forceActiveFocus()
            }

            CheckBox {
                id: checkBox
                Layout.fillWidth: true
                text: booleanEditorRoot.field ? booleanEditorRoot.field.label : ""
                checked: booleanEditorRoot.field ? booleanEditorRoot.field.value : false
            }

            Label {
                Layout.fillWidth: true
                visible: text.length !== 0
                text: booleanEditorRoot.field ? booleanEditorRoot.field.description : ""
                wrapMode: Text.Wrap
                ThemedItem.foregroundLevel: SVS.FL_Secondary
            }
        }
    }

    Component {
        id: choiceEditor

        ColumnLayout {
            id: choiceEditorRoot
            property var field

            Layout.fillWidth: true
            spacing: 4

            readonly property bool valid: !field || !field.required
                || (comboBox.currentIndex >= 0 && field.items[comboBox.currentIndex].enabled)

            function initialIndex() {
                if (!field)
                    return -1
                for (let index = 0; index < field.items.length; ++index) {
                    if (field.items[index].enabled && field.items[index].value === field.value)
                        return index
                }
                return -1
            }

            function validate() {
                return valid
            }

            function resultValue() {
                return comboBox.currentIndex >= 0 && field.items[comboBox.currentIndex].enabled
                    ? field.items[comboBox.currentIndex].value
                    : ""
            }

            function focusEditor() {
                comboBox.forceActiveFocus()
            }

            Label {
                Layout.fillWidth: true
                text: choiceEditorRoot.field ? choiceEditorRoot.field.label : ""
                wrapMode: Text.Wrap
            }

            ComboBox {
                id: comboBox
                property int lastEnabledIndex: choiceEditorRoot.initialIndex()

                Layout.fillWidth: true
                model: choiceEditorRoot.field ? choiceEditorRoot.field.items : []
                textRole: "label"
                currentIndex: choiceEditorRoot.initialIndex()
                onActivated: index => {
                    if (model[index].enabled)
                        lastEnabledIndex = index
                    else
                        currentIndex = lastEnabledIndex
                }

                delegate: ItemDelegate {
                    required property var modelData
                    width: comboBox.width
                    enabled: modelData.enabled
                    text: modelData.label
                }
            }

            Label {
                Layout.fillWidth: true
                visible: text.length !== 0
                text: comboBox.currentIndex >= 0 ? choiceEditorRoot.field.items[comboBox.currentIndex].description : ""
                wrapMode: Text.Wrap
                ThemedItem.foregroundLevel: SVS.FL_Secondary
            }

            Label {
                Layout.fillWidth: true
                visible: text.length !== 0
                text: choiceEditorRoot.field ? choiceEditorRoot.field.description : ""
                wrapMode: Text.Wrap
                ThemedItem.foregroundLevel: SVS.FL_Secondary
            }
        }
    }

    Component {
        id: colorEditor

        ColumnLayout {
            id: colorEditorRoot
            property var field

            Layout.fillWidth: true
            spacing: 4

            function validate() {
                return true
            }

            function resultValue() {
                return colorComboBox.color
            }

            function focusEditor() {
                colorComboBox.forceActiveFocus()
            }

            Label {
                Layout.fillWidth: true
                text: colorEditorRoot.field ? colorEditorRoot.field.label : ""
                wrapMode: Text.Wrap
            }

            ColorComboBox {
                id: colorComboBox
                Layout.fillWidth: true
                color: colorEditorRoot.field ? colorEditorRoot.field.value : "black"
                flags: SVS.CM_ColorSpecRgb
                    | SVS.CM_ColorSpecHsv
                    | SVS.CM_ColorSpecHsl
                    | SVS.CM_Hex
                    | SVS.CM_AxisChangeable
                    | SVS.CM_Eyedropper
                    | SVS.CM_NativeColorDialog
                    | (colorEditorRoot.field && colorEditorRoot.field.alpha ? SVS.CM_Alpha : 0)
            }

            Label {
                Layout.fillWidth: true
                visible: text.length !== 0
                text: colorEditorRoot.field ? colorEditorRoot.field.description : ""
                wrapMode: Text.Wrap
                ThemedItem.foregroundLevel: SVS.FL_Secondary
            }
        }
    }
}
