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

        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true

            TextArea {
                id: lyricsTextArea
                text: dialog.lyricsText
                wrapMode: TextEdit.Wrap
                onTextChanged: dialog.lyricsText = text
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
                onToggled: dialog.truncateToSelection = checked
            }
        }
    }

    standardButtons: DialogButtonBox.Ok
}
