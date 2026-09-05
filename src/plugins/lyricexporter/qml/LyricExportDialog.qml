// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SVSCraft
import SVSCraft.UIComponents

Window {
    id: dialog

    required property LyricExportSession session

    readonly property bool isMacOS: Qt.platform.os === "osx" || Qt.platform.os === "macos"
    property int currentStep: 0

    width: 880
    height: 640
    minimumWidth: 680
    minimumHeight: 480
    flags: Qt.Dialog | Qt.CustomizeWindowHint | Qt.WindowTitleHint
           | Qt.WindowSystemMenuHint | Qt.WindowCloseButtonHint
    modality: Qt.WindowModal
    title: qsTr("Export Lyrics")
    color: Theme.backgroundPrimaryColor
    LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
    LayoutMirroring.childrenInherit: true

    signal finished()

    onClosing: finished()

    function stepTitle() {
        switch (currentStep) {
        case 0:
            return qsTr("Select Track")
        case 1:
            return qsTr("Edit Line Breaks")
        case 2:
            return qsTr("Edit Lyrics and Time Codes")
        }
        return ""
    }

    function stepDescription() {
        switch (currentStep) {
        case 0:
            return qsTr("Choose the track whose lyrics you want to export.")
        case 1:
            return qsTr("Adjust the lyric line breaks and choose how the lyrics are processed.")
        case 2:
            return qsTr("Review and edit each lyric line and its time codes before exporting.")
        }
        return ""
    }

    function addFilterFromInput() {
        const value = filterInput.text.trim()
        if (value.length === 0 || session.filters.indexOf(value) >= 0)
            return
        session.addFilter(value)
        filterInput.clear()
    }

    function goBack() {
        if (currentStep === 0 || session.exportState === LyricExportSession.Exporting) {
            return
        }
        currentStep -= 1
    }

    function goForward() {
        if (currentStep === 0) {
            if (session.selectedTrack >= 0)
                currentStep = 1
            return
        }
        if (currentStep === 1) {
            if (session.prepareTable())
                currentStep = 2
            return
        }
        if (currentStep === 2) {
            session.exportFile()
        }
    }

    Connections {
        target: dialog.session

        function onExportStateChanged() {
            if (dialog.session.exportState === LyricExportSession.Succeeded)
                dialog.close()
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            Layout.fillWidth: true
            implicitHeight: titleLayout.implicitHeight + (dialog.isMacOS ? 36 : 24)
            color: Theme.backgroundPrimaryColor

            ColumnLayout {
                id: titleLayout

                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.leftMargin: dialog.isMacOS ? 24 : 12
                anchors.rightMargin: dialog.isMacOS ? 24 : 12
                anchors.topMargin: dialog.isMacOS ? 24 : 12
                spacing: 2

                Label {
                    Layout.fillWidth: true
                    text: dialog.stepTitle()
                    font.pixelSize: 20
                    font.weight: Font.DemiBold
                }

                Label {
                    Layout.fillWidth: true
                    text: dialog.stepDescription()
                    wrapMode: Text.Wrap
                    ThemedItem.foregroundLevel: SVS.FL_Secondary
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            implicitHeight: 1
            color: Theme.paneSeparatorColor
        }

        StackLayout {
            id: pageStack

            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: dialog.currentStep

            Item {
                id: trackPage

                Label {
                    id: noTrackLabel

                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.margins: 12
                    height: visible ? implicitHeight : 0
                    visible: dialog.session.selectedTrack < 0
                    text: qsTr("No track is available for lyric export.")
                    wrapMode: Text.Wrap
                    ThemedItem.foregroundLevel: SVS.FL_Secondary
                }

                ListView {
                    id: trackList

                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: noTrackLabel.bottom
                    anchors.bottom: parent.bottom
                    anchors.leftMargin: 12
                    anchors.rightMargin: 12
                    anchors.topMargin: noTrackLabel.visible ? 8 : 12
                    anchors.bottomMargin: 12
                    clip: true
                    spacing: 4
                    boundsBehavior: Flickable.StopAtBounds
                    reuseItems: true
                    model: dialog.session.trackModel

                    delegate: ItemDelegate {
                        id: trackDelegate

                        required property int index
                        required property string displayText
                        required property bool selectable
                        required property string warningText

                        width: ListView.view.width
                        enabled: selectable
                        highlighted: dialog.session.selectedTrack === index
                        Accessible.role: Accessible.ListItem
                        Accessible.name: displayText
                        Accessible.description: warningText

                        onClicked: {
                            if (selectable)
                                dialog.session.selectedTrack = index
                        }

                        contentItem: ColumnLayout {
                            spacing: 2

                            RadioButton {
                                Layout.fillWidth: true
                                checked: dialog.session.selectedTrack === trackDelegate.index
                                enabled: trackDelegate.selectable
                                text: trackDelegate.displayText
                                onClicked: dialog.session.selectedTrack = trackDelegate.index
                            }

                            Label {
                                Layout.fillWidth: true
                                Layout.leftMargin: trackDelegate.mirrored ? 0 : 32
                                Layout.rightMargin: trackDelegate.mirrored ? 32 : 0
                                visible: !trackDelegate.selectable
                                text: trackDelegate.warningText
                                color: Theme.warningColor
                                wrapMode: Text.Wrap
                            }
                        }
                    }
                }
            }

            Item {
                id: lineBreakPage

                GridLayout {
                    anchors.fill: parent
                    anchors.margins: 12
                    columns: 1
                    columnSpacing: 0
                    rowSpacing: 6

                    CheckBox {
                        Layout.row: 1
                        text: qsTr("Use spaces to separate words")
                        checked: dialog.session.useSpaces
                        onClicked: dialog.session.useSpaces = checked
                    }

                    Label {
                        Layout.row: 2
                        text: qsTr("Filter lyrics")
                    }

                    Frame {
                        id: filterField

                        Layout.row: 3
                        Layout.fillWidth: true
                        padding: 4
                        implicitHeight: Math.max(40, tagFlow.implicitHeight + topPadding + bottomPadding)

                        Flow {
                            id: tagFlow

                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.verticalCenter: parent.verticalCenter
                            spacing: 4

                            Repeater {
                                model: dialog.session.filters

                                delegate: Rectangle {
                                    id: filterTag

                                    required property string modelData

                                    implicitWidth: tagLayout.implicitWidth + 12
                                    implicitHeight: 28
                                    color: Theme.accentColor
                                    radius: 4

                                    RowLayout {
                                        id: tagLayout

                                        anchors.fill: parent
                                        anchors.leftMargin: 8
                                        spacing: 2

                                        Label {
                                            text: filterTag.modelData
                                            color: Theme.foregroundPrimaryColor
                                        }

                                        ToolButton {
                                            readonly property string accessibleText: qsTr("Remove Filter %1").arg(filterTag.modelData)

                                            Layout.preferredWidth: 20
                                            Layout.preferredHeight: 20
                                            Layout.alignment: Qt.AlignVCenter
                                            flat: true
                                            text: accessibleText
                                            display: AbstractButton.IconOnly
                                            icon.source: "image://fluent-system-icons/dismiss"
                                            icon.width: 12
                                            icon.height: 12
                                            Accessible.name: accessibleText
                                            ToolTip.visible: hovered || activeFocus
                                            ToolTip.text: accessibleText
                                            onClicked: dialog.session.removeFilter(filterTag.modelData)
                                        }
                                    }
                                }
                            }

                            TextField {
                                id: filterInput

                                width: 180
                                placeholderText: qsTr("Add a filter")
                                Accessible.name: qsTr("Filter lyrics")
                                onAccepted: dialog.addFilterFromInput()
                            }

                            ToolButton {
                                text: qsTr("Add Filter")
                                display: AbstractButton.IconOnly
                                icon.source: "image://fluent-system-icons/add"
                                Accessible.name: text
                                ToolTip.visible: hovered || activeFocus
                                ToolTip.text: text
                                onClicked: dialog.addFilterFromInput()
                            }
                        }
                    }

                    ListView {
                        id: lyricLineList

                        Layout.row: 0
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        spacing: 2
                        cacheBuffer: 0
                        reuseItems: true
                        boundsBehavior: Flickable.StopAtBounds
                        model: dialog.session.lineModel

                        delegate: Rectangle {
                            id: lineDelegate

                            required property int index
                            required property var words
                            required property bool interlude
                            required property bool canInsertInterlude
                            required property bool canRemoveBreak

                            width: ListView.view.width
                            implicitHeight: Math.max(28, lyricFlow.implicitHeight + 4)
                            color: index % 2 === 0
                                   ? Theme.backgroundSecondaryColor
                                   : Theme.backgroundPrimaryColor

                            Flow {
                                id: lyricFlow

                                anchors.left: parent.left
                                anchors.right: parent.right
                                anchors.top: parent.top
                                anchors.margins: 2
                                spacing: 0

                                Label {
                                    visible: lineDelegate.interlude
                                    text: qsTr("Interlude")
                                    font.italic: true
                                    color: Theme.foregroundDisabledColorChange.apply(
                                               Theme.foregroundPrimaryColor)
                                    leftPadding: 4
                                    rightPadding: 4
                                    height: 24
                                    verticalAlignment: Text.AlignVCenter
                                }

                                Repeater {
                                    model: lineDelegate.words

                                    delegate: Item {
                                        id: wordDelegate

                                        required property int index
                                        required property var modelData

                                        implicitWidth: wordLayout.implicitWidth
                                        implicitHeight: 24

                                        Row {
                                            id: wordLayout

                                            height: 24

                                            Item {
                                                visible: wordDelegate.index > 0
                                                width: visible ? 20 : 0
                                                height: 24

                                                ToolButton {
                                                    id: wordGapButton

                                                    anchors.centerIn: parent
                                                    width: 20
                                                    height: 20
                                                    text: qsTr("Insert Line Break")
                                                    display: AbstractButton.IconOnly
                                                    icon.source: "image://fluent-system-icons/add"
                                                    icon.width: 12
                                                    icon.height: 12
                                                    contentItem.opacity: hovered || activeFocus ? 1 : 0
                                                    ThemedItem.controlType: SVS.CT_Accent
                                                    Accessible.name: text
                                                    ToolTip.visible: hovered || activeFocus
                                                    ToolTip.text: text
                                                    onClicked: dialog.session.insertBreak(
                                                                   lineDelegate.index,
                                                                   wordDelegate.index - 1)
                                                }
                                            }

                                            Label {
                                                height: 24
                                                leftPadding: 2
                                                rightPadding: 2
                                                verticalAlignment: Text.AlignVCenter
                                                text: wordDelegate.modelData.text.length > 0
                                                      ? wordDelegate.modelData.text
                                                      : qsTr("Empty Lyric")
                                                font.italic: wordDelegate.modelData.text.length === 0
                                                color: wordDelegate.modelData.filtered
                                                       ? Theme.foregroundDisabledColorChange.apply(
                                                             Theme.foregroundPrimaryColor)
                                                       : Theme.foregroundPrimaryColor
                                            }
                                        }
                                    }
                                }

                                Item {
                                    visible: lineDelegate.canInsertInterlude
                                    width: visible ? 20 : 0
                                    height: 24

                                    ToolButton {
                                        id: interludeButton

                                        anchors.centerIn: parent
                                        width: 20
                                        height: 20
                                        text: qsTr("Insert Interlude")
                                        display: AbstractButton.IconOnly
                                        icon.source: "image://fluent-system-icons/add"
                                        icon.width: 12
                                        icon.height: 12
                                        contentItem.opacity: hovered || activeFocus ? 1 : 0
                                        ThemedItem.controlType: SVS.CT_Accent
                                        Accessible.name: text
                                        ToolTip.visible: hovered || activeFocus
                                        ToolTip.text: text
                                        onClicked: dialog.session.insertInterlude(lineDelegate.index)
                                    }
                                }

                                Item {
                                    width: 20
                                    height: 24

                                    ToolButton {
                                        id: lineBreakButton

                                        readonly property string accessibleText: enabled ? qsTr("Remove Line Break") : qsTr("End of Lyrics")
                                        readonly property bool keyboardFocused: activeFocus
                                                                                 && (focusReason === Qt.TabFocusReason
                                                                                     || focusReason === Qt.BacktabFocusReason
                                                                                     || focusReason === Qt.ShortcutFocusReason)
                                        readonly property bool showRemoveIcon: enabled && (hovered || keyboardFocused)

                                        anchors.centerIn: parent
                                        width: 20
                                        height: 20
                                        enabled: lineDelegate.canRemoveBreak
                                        text: accessibleText
                                        display: AbstractButton.IconOnly
                                        icon.source: showRemoveIcon
                                                     ? "image://fluent-system-icons/subtract"
                                                     : "image://fluent-system-icons/arrow_enter_left"
                                        icon.width: 12
                                        icon.height: 12
                                        ThemedItem.controlType: SVS.CT_Error
                                        Accessible.name: accessibleText
                                        ToolTip.visible: showRemoveIcon
                                        ToolTip.text: accessibleText
                                        onClicked: dialog.session.removeBreak(lineDelegate.index)
                                    }
                                }
                            }
                        }
                    }
                }
            }

            Item {
                id: tablePage

                Item {
                    anchors.fill: parent
                    anchors.margins: 12

                    HorizontalHeaderView {
                        id: tableHeader

                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.top: parent.top
                        syncView: lyricTable
                        clip: true
                    }

                    TableView {
                        id: lyricTable

                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.top: tableHeader.bottom
                        anchors.bottom: validationLabel.top
                        clip: true
                        reuseItems: true
                        boundsBehavior: Flickable.StopAtBounds
                        model: dialog.session.tableModel
                        columnSpacing: 1
                        rowSpacing: 1
                        columnWidthProvider: column => {
                            if (column === dialog.session.lyricsColumn)
                                return Math.max(320, lyricTable.width - (lyricTable.columns - 1) * 160)
                            return 160
                        }
                        rowHeightProvider: () => 40

                        delegate: Rectangle {
                            id: tableCell

                            required property int row
                            required property int column
                            required property var display

                            implicitWidth: column === dialog.session.lyricsColumn ? 480 : 160
                            implicitHeight: 40
                            color: row % 2 === 0
                                   ? Theme.backgroundSecondaryColor
                                   : Theme.backgroundPrimaryColor

                            TextField {
                                id: cellEditor

                                anchors.fill: parent
                                anchors.margins: 2
                                text: tableCell.display ?? ""
                                Accessible.name: tableCell.column === dialog.session.lyricsColumn
                                                 ? qsTr("Lyrics")
                                                 : tableCell.column === 0
                                                   ? qsTr("Start") : qsTr("End")
                                onEditingFinished: {
                                    dialog.session.setTableCell(tableCell.row, tableCell.column, text)
                                    text = Qt.binding(() => tableCell.display ?? "")
                                }
                            }
                        }
                    }

                    Label {
                        anchors.centerIn: lyricTable
                        visible: dialog.session.tableRowCount === 0
                        text: qsTr("No lyrics remain after filtering.")
                        ThemedItem.foregroundLevel: SVS.FL_Secondary
                    }

                    Label {
                        id: validationLabel

                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.bottom: parent.bottom
                        height: visible ? implicitHeight + 8 : 0
                        visible: dialog.session.validationMessage.length > 0
                        text: dialog.session.validationMessage
                        color: Theme.warningColor
                        wrapMode: Text.Wrap
                    }
                }
            }

        }

        Rectangle {
            Layout.fillWidth: true
            implicitHeight: 1
            color: Theme.paneSeparatorColor
        }

        Rectangle {
            Layout.fillWidth: true
            height: dialog.isMacOS ? 64 : 52
            color: Theme.backgroundSecondaryColor

            RowLayout {
                anchors.fill: parent
                anchors.margins: dialog.isMacOS ? 24 : 12
                anchors.topMargin: 12
                spacing: 12

                Button {
                    visible: dialog.currentStep > 0
                    enabled: dialog.session.exportState !== LyricExportSession.Exporting
                    text: qsTr("Previous")
                    onClicked: dialog.goBack()
                }

                Item {
                    Layout.fillWidth: true
                }

                Button {
                    id: forwardButton

                    ThemedItem.controlType: SVS.CT_Accent
                    enabled: {
                        if (dialog.currentStep === 0)
                            return dialog.session.selectedTrack >= 0
                        return dialog.session.exportState !== LyricExportSession.Exporting
                    }
                    text: dialog.currentStep === 2 ? qsTr("Export") : qsTr("Next")
                    onClicked: dialog.goForward()
                }

                Button {
                    enabled: dialog.session.exportState !== LyricExportSession.Exporting
                    text: qsTr("Cancel")
                    onClicked: dialog.close()
                }
            }
        }
    }
}
