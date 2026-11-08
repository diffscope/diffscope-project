// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SVSCraft
import SVSCraft.UIComponents
import SVSCraft.UIComponents.impl

import DiffScope.UIShell

AbstractPropertyEditorField {
    id: languageField
    required property var languageOptions

    FormGroup {
        Layout.fillWidth: true
        label: languageField.label
        rowItem: ToolButton {
            text: qsTr("Enter Custom Language Code")
            display: AbstractButton.IconOnly
            icon.source: "image://fluent-system-icons/edit"
            onClicked: () => {
                const warningText = qsTr("Custom language codes may not be supported by the singer.")
                const currentLanguage = languageField.value
                const initialLanguage = typeof currentLanguage === "string"
                                        ? currentLanguage
                                        : ""
                const languageCode = languageField.windowHandle.execQuickInput(
                    qsTr("Custom Language Code"),
                    warningText,
                    initialLanguage,
                    (text, attempted) => {
                        const empty = text.trim() === ""
                        return {
                            acceptable: !empty,
                            status: empty
                                    ? (attempted ? SVS.CT_Error : SVS.CT_Normal)
                                    : SVS.CT_Warning,
                            promptText: empty ? qsTr("Language code should not be empty") : warningText
                        }
                    }
                )
                if (typeof languageCode === "string")
                    languageField.setValue(languageCode)
            }
        }
        columnItem: ComboBox {
            id: languageComboBox
            model: languageField.languageOptions
            textRole: "text"
            valueRole: "value"
            readonly property int modelIndex: {
                const language = languageField.value
                return typeof language !== "string"
                        ? -1
                        : languageField.languageOptions.findIndex(option => option.value === language)
            }
            currentIndex: modelIndex
            displayText: {
                const language = languageField.value
                if (typeof language !== "string")
                    return qsTr("Multiple values")

                const option = languageField.languageOptions.find(option => option.value === language)
                return option ? option.text : qsTr("Custom (%1)").arg(language)
            }
            onActivated: (index) => {
                if (!languageField.setValue(valueAt(index)))
                    GlobalHelper.setProperty(languageComboBox, "currentIndex", modelIndex)
            }
        }
    }
}
