import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SVSCraft
import SVSCraft.UIComponents

import DiffScope.Core

Dialog {
    id: dialog

    required property QtObject addOn
    property string lyricsText
    property int splitMode
    property string regularExpression
    property bool truncateToSelection

    readonly property bool regularExpressionValid: addOn.isRegularExpressionValid(regularExpression)

    width: 480
    height: 560
    title: qsTr("Fill Lyrics")
    modal: true
    closePolicy: Popup.CloseOnEscape

    function selectSplitMode(mode) {
        if (mode !== FillLyricsAddOn.SplitMode_Regex)
            regularExpression = addOn.regularExpressionForSplitMode(mode)
        splitMode = mode
    }

    onAboutToShow: lyricsTextArea.forceActiveFocus()

    ColumnLayout {
        anchors.fill: parent
        spacing: 12

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 0
            TabBar {
                id: editModeTabBar
                ThemedItem.flat: true
                TabButton {
                    text: qsTr("Edit")
                    ThemedItem.tabIndicator: SVS.TI_Bottom
                }
                TabButton {
                    text: qsTr("Preview")
                    ThemedItem.tabIndicator: SVS.TI_Bottom
                }
            }
            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: Theme.borderColor
            }
        }

        StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: editModeTabBar.currentIndex
            ScrollView {
                TextArea {
                    id: lyricsTextArea
                    text: dialog.lyricsText
                    wrapMode: TextEdit.Wrap
                    onTextChanged: dialog.lyricsText = text
                }
            }
            ColumnLayout {
                ScrollView {
                    id: previewScrollView
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Flow {
                        width: previewScrollView.width - 8
                        spacing: 8
                        Repeater {
                            id: previewRepeater
                            property string previewLyricsText
                            property string previewRegularExpression
                            property bool previewTruncateToSelection
                            readonly property Binding binding: Binding {
                                when: editModeTabBar.currentIndex === 1
                                previewRepeater.previewLyricsText: dialog.lyricsText
                                previewRepeater.previewRegularExpression: dialog.regularExpression
                                previewRepeater.previewTruncateToSelection: dialog.truncateToSelection
                            }
                            readonly property list<string> lyrics: dialog.addOn.splitLyrics(previewLyricsText, previewRegularExpression)
                            model: lyrics
                            delegate: Frame {
                                id: previewFrame
                                required property int index
                                required property string modelData
                                padding: 4
                                Label {
                                    text: previewFrame.modelData
                                }
                                background: Rectangle {
                                    color: Theme.textFieldColor
                                    border.width: 1
                                    border.color: (previewRepeater.previewTruncateToSelection && previewFrame.index >= dialog.addOn.windowHandle.projectDocumentContext.document.selectionModel.selectedCount) ? Theme.warningColor : Theme.borderColor
                                }
                            }
                        }
                    }
                }
                Label {
                    Layout.alignment: Qt.AlignRight
                    ThemedItem.foregroundLevel: SVS.FL_Secondary
                    text: qsTr("%Ln word(s)", "", previewRepeater.lyrics.length)
                }
            }
        }

        GridLayout {
            Layout.fillWidth: true
            columns: 2

            Label {
                id: splitModeLabel
                text: qsTr("Split mode")
            }
            TabBar {
                Layout.fillWidth: true
                currentIndex: dialog.splitMode

                TabButton {
                    icon.source: "image://fluent-system-icons/local_language_zi"
                    text: qsTr("Auto")
                    onClicked: dialog.selectSplitMode(FillLyricsAddOn.SplitMode_Auto)
                }
                TabButton {
                    icon.source: "image://fluent-system-icons/square_zi"
                    text: qsTr("Character")
                    onClicked: dialog.selectSplitMode(FillLyricsAddOn.SplitMode_Character)
                }
                TabButton {
                    icon.source: "image://fluent-system-icons/text_whole_word"
                    text: qsTr("Word")
                    onClicked: dialog.selectSplitMode(FillLyricsAddOn.SplitMode_Word)
                }
                TabButton {
                    icon.source: "image://fluent-system-icons/text_period_asterisk"
                    text: qsTr("Regex")
                    onClicked: dialog.selectSplitMode(FillLyricsAddOn.SplitMode_Regex)
                }
            }

            Label {
                id: regularExpressionLabel
                text: qsTr("Regex")
            }
            TextField {
                id: regularExpressionField
                Accessible.labelledBy: regularExpressionLabel
                Accessible.name: regularExpressionLabel.text
                Layout.fillWidth: true
                enabled: dialog.splitMode === FillLyricsAddOn.SplitMode_Regex
                text: dialog.regularExpression
                ThemedItem.controlType: enabled && !dialog.regularExpressionValid ? SVS.CT_Error : SVS.CT_Normal
                onTextEdited: dialog.regularExpression = text
            }

            CheckBox {
                Layout.columnSpan: 2
                text: qsTr("Truncate to selection")
                checked: dialog.truncateToSelection
                onClicked: dialog.truncateToSelection = checked
            }
        }
    }

    standardButtons: DialogButtonBox.Ok
}
