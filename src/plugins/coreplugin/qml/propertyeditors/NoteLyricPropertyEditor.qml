import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SVSCraft
import SVSCraft.UIComponents

import DiffScope.UIShell
import DiffScope.Core

PropertyEditorGroupBox {
    id: groupBox
    required property ProjectWindowInterface windowHandle
    required property QtObject propertyMapper
    readonly property var singingClip: propertyMapper?.singingClip ?? null
    readonly property string firstLeafSingerId: findFirstLeafSingerId(clipSingerIdProvider.singerTree)
    readonly property var languageOptions: singerInfoProvider.languageOptions
    readonly property bool useGlobalLanguageFallback: !singerInfoProvider.exists || languageOptions.length === 0
    title: qsTr("Lyric and Pronunciation")

    function findFirstLeafSingerId(singerTree) {
        if (typeof singerTree === "string")
            return singerTree
        if (!singerTree || typeof singerTree.length !== "number")
            return ""

        for (let index = 0; index < singerTree.length; ++index) {
            const singerId = findFirstLeafSingerId(singerTree[index])
            if (singerId !== "")
                return singerId
        }
        return ""
    }

    ClipSingerIdProvider {
        id: clipSingerIdProvider
        sources: groupBox.singingClip?.sources ?? null
    }

    SingerInfoProvider {
        id: singerInfoProvider
        registry: CoreInterface.singerRegistry
        architectureId: clipSingerIdProvider.architectureId
        singerId: groupBox.firstLeafSingerId
    }

    ColumnLayout {
        width: parent.width

        TextPropertyEditorField {
            windowHandle: groupBox.windowHandle
            propertyMapper: groupBox.propertyMapper
            key: "lyric"
            label: qsTr("Lyric")
            transactionName: qsTr("Editing lyric")
        }

        AbstractPropertyEditorField {
            id: languageField
            windowHandle: groupBox.windowHandle
            propertyMapper: groupBox.propertyMapper
            key: "language"
            label: qsTr("Language")
            transactionName: qsTr("Editing language")

            FormGroup {
                Layout.fillWidth: true
                label: languageField.label
                rowItem: ToolButton {
                    text: qsTr("Enter custom language code")
                    flat: true
                    display: AbstractButton.IconOnly
                    icon.source: "image://fluent-system-icons/edit"
                    ToolTip.visible: hovered
                    ToolTip.text: text
                    onClicked: () => {
                        const warningText = qsTr("Custom language codes may not be supported by the singer.")
                        const currentLanguage = groupBox.propertyMapper?.language
                        const initialLanguage = typeof currentLanguage === "string"
                                                ? currentLanguage
                                                : BehaviorPreference.fallbackLyricLanguageCode
                        const languageCode = groupBox.windowHandle.execQuickInput(
                            qsTr("Custom language code"),
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
                    model: groupBox.languageOptions
                    textRole: "text"
                    valueRole: "value"
                    currentIndex: {
                        groupBox.languageOptions
                        const language = groupBox.propertyMapper?.language
                        return groupBox.useGlobalLanguageFallback || typeof language !== "string"
                                ? -1
                                : indexOfValue(language)
                    }
                    displayText: {
                        groupBox.languageOptions
                        if (groupBox.useGlobalLanguageFallback)
                            return BehaviorPreference.fallbackLyricLanguageCode

                        const language = groupBox.propertyMapper?.language
                        if (typeof language !== "string")
                            return qsTr("Multiple values")

                        const index = indexOfValue(language)
                        return index === -1 ? qsTr("Custom (%1)").arg(language) : textAt(index)
                    }
                    onActivated: (index) => languageField.setValue(valueAt(index))
                }
            }
        }

        FormGroup {
            Layout.fillWidth: true
            label: qsTr("Pronunciation (original)")
            columnItem: TextField {
                text: groupBox.propertyMapper?.originalPronunciation === undefined
                      ? qsTr("Multiple values")
                      : groupBox.propertyMapper?.originalPronunciation ?? ""
                readOnly: true
                ThemedItem.flat: true
            }
        }

        TextPropertyEditorField {
            windowHandle: groupBox.windowHandle
            propertyMapper: groupBox.propertyMapper
            key: "editedPronunciation"
            label: qsTr("Pronunciation (edited)")
            transactionName: qsTr("Editing pronunciation (edited)")
        }
    }
}
