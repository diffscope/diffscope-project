import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQml.Models

import SVSCraft
import SVSCraft.UIComponents

Frame {
    id: frame
    padding: 0
    property bool autoWrap: false
    property QtObject model: null
    focusPolicy: Qt.StrongFocus
    Accessible.name: qsTr("Lyric Pronunciation Editor")
    component LyricCellDelegate: ColumnLayout {
        id: cell
        property string pronunciation: ""
        property string lyric: ""
        readonly property bool highlighted: row === lyricTable.currentIndexRow && column === lyricTable.currentIndexColumn
        required property int row
        required property int column
        property list<string> candidatePronunciations: []
        Accessible.role: Accessible.Cell
        Accessible.name: lyric
        Accessible.description: qsTr("Current pronunciation is %1. Candidate pronunciations are %2. Activate to modify pronunciation.").replace("%1", pronunciation).replace("%2", candidatePronunciations.join(" "))
        Accessible.checkable: true
        Accessible.checked: highlighted
        spacing: 1
        Accessible.onPressAction: openContextMenu()
        Accessible.onToggleAction: openContextMenu()

        function activate() {
            lyricTable.currentIndexRow = row
            lyricTable.currentIndexColumn = column
        }
        function openContextMenu() {
            activate()
            contextMenu.popup()
        }
        function modifyLyric() {
            modifyLyricPopup.open()
        }
        Popup {
            id: customizePronunciationPopup
            closePolicy: Popup.CloseOnPressOutside | Popup.CloseOnEscape
            y: parent.height + 4
            onAboutToShow: () => {
                customizePronunciationEdit.text = cell.pronunciation
                customizePronunciationEdit.forceActiveFocus()
            }
            ColumnLayout {
                spacing: 4
                Label {
                    id: customizePronunciationLabel
                    Accessible.ignored: true
                    Layout.minimumWidth: 200
                    Layout.fillWidth: true
                    text: qsTr('Customize pronunciation of "%1"').replace("%1", cell.lyric)
                }
                TextField {
                    id: customizePronunciationEdit
                    Accessible.description: customizePronunciationLabel.text
                    Layout.fillWidth: true
                    Keys.onReturnPressed: customizePronunciationSubmitButton.animateClick()
                }
                Button {
                    id: customizePronunciationSubmitButton
                    Layout.fillWidth: true
                    text: qsTr("OK")
                    onClicked: () => {
                        frame.model.setData(frame.model.index(column, 0, frame.model.index(row, 0)), customizePronunciationEdit.text, USDef.LC_PronunciationRole)
                        customizePronunciationPopup.close()
                    }
                }
            }
        }
        Popup {
            id: modifyLyricPopup
            property bool escapePressed: false
            parent: lyricRectangle
            closePolicy: Popup.CloseOnPressOutside
            padding: 0
            background: Item {}
            onAboutToShow: () => {
                modifyLyricEdit.text = cell.lyric
                modifyLyricEdit.forceActiveFocus()
            }
            onAboutToHide: () => {
                if (escapePressed) {
                    escapePressed = false
                    return
                }
                frame.model.setData(frame.model.index(column, 0, frame.model.index(row, 0)), modifyLyricEdit.text, USDef.LC_LyricRole)
            }
            TextField {
                id: modifyLyricEdit
                leftPadding: 6
                rightPadding: 6
                background.implicitWidth: 0
                Accessible.description: qsTr("Edit lyric")
                Keys.onReturnPressed: modifyLyricPopup.close()
                Keys.onEscapePressed: () => {
                    modifyLyricPopup.escapePressed = true
                    modifyLyricPopup.close
                }
            }
        }
        Menu {
            id: contextMenu
            Instantiator {
                model: cell.candidatePronunciations
                delegate: Action {
                    required property int index
                    checkable: true
                    checked: cell.pronunciation === cell.candidatePronunciations[index]
                    text: cell.candidatePronunciations[index]
                    onTriggered: () => {
                        frame.model.setData(frame.model.index(column, 0, frame.model.index(row, 0)), cell.candidatePronunciations[index], USDef.LC_PronunciationRole)
                    }
                }
                onObjectAdded: (index, action) => contextMenu.insertAction(index, action)
                onObjectRemoved: (index, action) => contextMenu.removeAction(action)
            }
            Action {
                text: qsTr("Customize Pronunciation...")
                onTriggered: customizePronunciationPopup.open()
            }
            MenuSeparator {
            }
            Action {
                text: qsTr("Edit Lyric...")
                onTriggered: cell.modifyLyric()
            }
            MenuSeparator {
            }
            Action {
                text: qsTr("Insert Cell Before")
                onTriggered: () => {
                    frame.model.insertRow(cell.column, frame.model.index(cell.row, 0))
                }
            }
            Action {
                text: qsTr("Insert Cell After")
                onTriggered: () => {
                    lyricTable.currentIndexColumn++
                    frame.model.insertRow(cell.column + 1, frame.model.index(cell.row, 0))
                }
            }
            Action {
                text: qsTr("Delete Cell")
                onTriggered: () => {
                    lyricTable.currentIndexRow = lyricTable.currentIndexColumn = -1
                    frame.model.removeRow(cell.column, frame.model.index(cell.row, 0))
                }
            }
            MenuSeparator {
            }
            Action {
                text: qsTr("Insert Line Before")
                onTriggered: () => {
                    lyricTable.currentIndexRow++
                    frame.model.insertRow(cell.row)
                }
            }
            Action {
                text: qsTr("Insert Line After")
                onTriggered: () => {
                    frame.model.insertRow(cell.row + 1)
                }
            }
            Action {
                text: qsTr("Delete Line")
                onTriggered: () => {
                    lyricTable.currentIndexRow = lyricTable.currentIndexColumn = -1
                    frame.model.removeRow(cell.row)
                }
            }
        }
        TapHandler {
            acceptedButtons: Qt.LeftButton
            onSingleTapped: cell.activate()
            onDoubleTapped: cell.modifyLyric()
        }
        TapHandler {
            acceptedButtons: Qt.RightButton
            onSingleTapped: cell.openContextMenu()
        }

        Text {
            Layout.alignment: Qt.AlignHCenter
            font.pixelSize: 12
            text: cell.pronunciation
            color: cell.candidatePronunciations.indexOf(pronunciation) === -1 ? Theme.warningColor : cell.candidatePronunciations.length > 1 ? Theme.accentColor: Theme.foregroundPrimaryColor
        }
        Rectangle {
            id: lyricRectangle
            Layout.alignment: Qt.AlignHCenter
            implicitWidth: lyricText.implicitWidth + 12
            implicitHeight: 28
            color: cell.highlighted ? Theme.controlCheckedColorChange.apply(Theme.buttonColor) : Theme.backgroundPrimaryColor
            border.color: cell.highlighted ? Theme.accentColor : Theme.borderColor
            border.width: 2
            radius: 4
            Text {
                id: lyricText
                color: Theme.foregroundPrimaryColor
                anchors.centerIn: parent
                text: cell.lyric
            }
            Behavior on color {
                ColorAnimation {
                    duration: Theme.colorAnimationDuration
                    easing.type: Easing.OutCubic
                }
            }
            Behavior on border.color {
                ColorAnimation {
                    duration: Theme.colorAnimationDuration
                    easing.type: Easing.OutCubic
                }
            }
        }
    }
    ScrollView {
        id: scrollView
        anchors.fill: parent
        ColumnLayout {
            id: lyricTable
            property int currentIndexRow: -1
            property int currentIndexColumn: -1
            spacing: 0
            Repeater {
                id: lyricRowRepeater
                model: frame.model
                delegate: Item {
                    id: lyricRow
                    readonly property Repeater lyricCellRepeater: cellRepeater
                    Layout.fillWidth: true
                    Layout.leftMargin: 8
                    Layout.rightMargin: 8
                    implicitWidth: lyricRowLayout.implicitWidth
                    implicitHeight: Math.max(lyricRowLayout.implicitHeight, 57)
                    required property int index
                    MouseArea {
                        anchors.fill: parent
                        acceptedButtons: Qt.RightButton
                        onClicked: rowMenu.popup()
                    }
                    ColumnLayout {
                        id: lyricRowLayout
                        spacing: 0
                        height: parent.height
                        LyricsSubtreeProxyModel {
                            id: subtreeProxyModel
                            sourceModel: frame.model
                            rootIndex: frame.model.index(lyricRow.index, 0)
                        }
                        Flow {
                            id: lyricRowFlow
                            Accessible.role: Accessible.Row
                            Accessible.name: qsTr("Lyrics Line %1").replace("%1", (lyricRow.index + 1).toLocaleString())
                            Layout.topMargin: 8
                            Layout.bottomMargin: 8
                            Layout.alignment: Qt.AlignVCenter
                            property bool _f: false
                            Layout.preferredWidth: _f ? Number.MAX_VALUE : frame.autoWrap ? scrollView.width - 16 : Math.max(scrollView.width - 16, implicitWidth)
                            spacing: 8
                            Connections {
                                target: frame
                                function onAutoWrapChanged() {
                                    if (!frame.autoWrap) {
                                        lyricRowFlow._f = true
                                        lyricRowFlow.forceLayout()
                                    }
                                }
                            }
                            onPositioningComplete: () => {
                                _f = false
                            }
                            Repeater {
                                id: cellRepeater
                                model: subtreeProxyModel
                                delegate: LyricCellDelegate {
                                    id: cell
                                    required property QtObject modelData
                                    required property int index
                                    row: lyricRow.index
                                    column: index
                                    Layout.alignment: Qt.AlignVCenter
                                    pronunciation: modelData.pronunciation ?? ""
                                    lyric: modelData.lyric ?? ""
                                    candidatePronunciations: modelData.candidatePronunciations ?? []
                                    Connections {
                                        target: subtreeProxyModel
                                        function onDataChanged(topLeft, bottomRight) {
                                            if (cell.index >= row && cell.index <= bottomRight.row) {
                                                cell.pronunciation = modelData.pronunciation ?? ""
                                                cell.lyric = modelData.lyric ?? ""
                                                cell.candidatePronunciations = modelData.candidatePronunciations ?? []
                                            }
                                        }
                                    }
                                }
                            }
                        }
                        Rectangle {
                            Layout.fillWidth: true
                            implicitHeight: 1
                            color: Theme.borderColor
                        }
                    }
                    Menu {
                        id: rowMenu
                        Action {
                            text: qsTr("Append Cell")
                            onTriggered: () => {
                                frame.model.insertRow(frame.model.rowCount(frame.model.index(lyricRow.index, 0)), frame.model.index(lyricRow.index, 0))
                                lyricTable.currentIndexRow = lyricRow.index
                                lyricTable.currentIndexColumn = frame.model.rowCount(frame.model.index(lyricRow.index, 0)) - 1
                            }
                        }
                        MenuSeparator {
                        }
                        Action {
                            text: qsTr("Insert Line Before")
                            onTriggered: () => {
                                if (lyricTable.currentIndexRow >= lyricRow.index) {
                                    lyricTable.currentIndexRow++
                                }
                                frame.model.insertRow(lyricRow.index)
                            }
                        }
                        Action {
                            text: qsTr("Insert Line After")
                            onTriggered: () => {
                                if (lyricTable.currentIndexRow > lyricRow.index) {
                                    lyricTable.currentIndexRow++
                                }
                                frame.model.insertRow(lyricRow.index + 1)
                            }
                        }
                        Action {
                            text: qsTr("Delete Line")
                            onTriggered: () => {
                                if (lyricTable.currentIndexRow === lyricRow.index) {
                                    lyricTable.currentIndexRow = lyricTable.currentIndexColumn = -1
                                } else if (lyricTable.currentIndexRow > lyricRow.index) {
                                    lyricTable.currentIndexRow--
                                }
                                frame.model.removeRow(lyricRow.index)
                            }
                        }
                    }
                }
            }
        }
    }

    Keys.onLeftPressed: () => {
        if (lyricTable.currentIndexColumn > 0) {
            lyricTable.currentIndexColumn--
        } else if (lyricTable.currentIndexRow > 0) {
            let targetRow = lyricTable.currentIndexRow - 1
            while (targetRow > 0 && frame.model.rowCount(frame.model.index(targetRow, 0)) === 0) {
                targetRow--
            }
            if (frame.model.rowCount(frame.model.index(targetRow, 0)) !== 0) {
                lyricTable.currentIndexRow = targetRow
                lyricTable.currentIndexColumn = frame.model.rowCount(frame.model.index(lyricTable.currentIndexRow, 0)) - 1
            }

        }
    }

    Keys.onRightPressed: () => {
        if (lyricTable.currentIndexColumn < frame.model.rowCount(frame.model.index(lyricTable.currentIndexRow, 0)) - 1) {
            lyricTable.currentIndexColumn++
        } else if (lyricTable.currentIndexRow < frame.model.rowCount() - 1) {
            let targetRow = lyricTable.currentIndexRow + 1
            while (targetRow < frame.model.rowCount() - 1 && frame.model.rowCount(frame.model.index(targetRow, 0)) === 0) {
                targetRow++
            }
            if (frame.model.rowCount(frame.model.index(targetRow, 0)) !== 0) {
                lyricTable.currentIndexRow = targetRow
                lyricTable.currentIndexColumn = 0
            }
        }
    }

    Keys.onUpPressed: () => {
        if (lyricTable.currentIndexRow <= 0)
            return
        let targetRow = lyricTable.currentIndexRow - 1
        while (targetRow > 0 && frame.model.rowCount(frame.model.index(targetRow, 0)) === 0) {
            targetRow--
        }
        if (frame.model.rowCount(frame.model.index(targetRow, 0)) === 0) {
            return
        }
        lyricTable.currentIndexRow = targetRow
        if (lyricTable.currentIndexColumn >= frame.model.rowCount(frame.model.index(lyricTable.currentIndexRow, 0))) {
            lyricTable.currentIndexColumn = frame.model.rowCount(frame.model.index(lyricTable.currentIndexRow, 0)) - 1
        }
    }

    Keys.onDownPressed: () => {
        if (lyricTable.currentIndexRow >= frame.model.rowCount() - 1)
            return
        let targetRow = lyricTable.currentIndexRow + 1
        while (targetRow < frame.model.rowCount() - 1 && frame.model.rowCount(frame.model.index(targetRow, 0)) === 0) {
            targetRow++
        }
        if (frame.model.rowCount(frame.model.index(targetRow, 0)) === 0) {
            return
        }
        lyricTable.currentIndexRow = targetRow
        if (lyricTable.currentIndexColumn >= frame.model.rowCount(frame.model.index(lyricTable.currentIndexRow, 0))) {
            lyricTable.currentIndexColumn = frame.model.rowCount(frame.model.index(lyricTable.currentIndexRow, 0)) - 1
        }
    }

    Keys.onMenuPressed: () => {
        let cell = lyricRowRepeater.itemAt(lyricTable.currentIndexRow).lyricCellRepeater.itemAt(lyricTable.currentIndexColumn)
        cell.openContextMenu()
    }
}