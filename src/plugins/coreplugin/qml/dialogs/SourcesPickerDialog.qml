import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl
import QtQuick.Layouts
import QtQuick.Templates as T

import ChorusKit.AppCore

import SVSCraft
import SVSCraft.UIComponents

import DiffScope.UIShell
import DiffScope.Core

Window {
    id: dialog

    required property SourcesPickerModel sourcesModel

    property int currentRow: 0
    property bool resultSent: false

    readonly property bool needsInitialPicker: sourcesModel.architectureId === "" || sourcesModel.empty
    readonly property bool addPickerSelected: !needsInitialPicker && currentRow === sourcesModel.count
    readonly property var selectedModelIndex: {
        sourcesModel.revision
        return !needsInitialPicker
               && currentRow >= 0
               && currentRow < sourcesModel.count
               ? sourcesModel.modelIndex(currentRow)
               : null
    }
    readonly property bool selectedModelIndexValid: {
        sourcesModel.revision
        return Boolean(selectedModelIndex && sourcesModel.indexAlive(selectedModelIndex))
    }
    readonly property int selectedSingerType: {
        sourcesModel.revision
        return selectedModelIndexValid
               ? sourcesModel.singerType(selectedModelIndex)
               : SourcesPickerModel.InvalidSinger
    }

    width: 840
    height: 600
    minimumWidth: 680
    minimumHeight: 480
    flags: Qt.Dialog | Qt.CustomizeWindowHint | Qt.WindowTitleHint
           | Qt.WindowSystemMenuHint | Qt.WindowCloseButtonHint
    modality: Qt.ApplicationModal
    title: qsTr("Select Sources")

    WindowSystem.windowSystem: CoreInterface.windowSystem
    WindowSystem.id: "org.diffscope.core.sourcespickerdialog"

    signal accepted()
    signal rejected()
    signal finished()

    Item {
        id: messageBoxHost
        anchors.fill: parent
        visible: false
    }

    function acceptDialog() {
        resultSent = true
        accepted()
        close()
    }

    function rejectDialog() {
        resultSent = true
        rejected()
        close()
    }

    function htmlEscape(value) {
        return String(value).replace(/&/g, "&amp;")
                            .replace(/</g, "&lt;")
                            .replace(/>/g, "&gt;")
                            .replace(/\"/g, "&quot;")
    }

    function showValidationIssues() {
        const items = sourcesModel.validationIssues.map(issue => {
            const path = issue.path ? `<b>${htmlEscape(issue.path)}</b>: ` : ""
            return `<li>${path}${htmlEscape(issue.message)}</li>`
        }).join("")
        messageBoxHost.MessageBox.warning(qsTr("Source singer warnings"), `<ul>${items}</ul>`)
    }

    function handlePickedSinger(architectureId, singerId) {
        let success = false
        if (needsInitialPicker)
            success = sourcesModel.trySelectInitialSinger(architectureId, singerId)
        else if (addPickerSelected)
            success = sourcesModel.tryAppendSinger(architectureId, singerId)
        if (success)
            currentRow = Math.max(0, sourcesModel.count - 1)
    }

    function moveRootSinger(modelIndex, sourceRow, destinationRow) {
        if (sourceRow === destinationRow)
            return
        let nextCurrentRow = currentRow
        if (currentRow === sourceRow) {
            nextCurrentRow = destinationRow
        } else if (sourceRow < currentRow && destinationRow >= currentRow) {
            nextCurrentRow = currentRow - 1
        } else if (sourceRow > currentRow && destinationRow <= currentRow) {
            nextCurrentRow = currentRow + 1
        }
        if (sourcesModel.moveSinger(modelIndex, destinationRow))
            currentRow = nextCurrentRow
    }

    onSourcesModelChanged: {
        if (sourcesModel)
            sourcesModel.registry = CoreInterface.singerRegistry
    }
    onVisibleChanged: {
        if (visible) {
            resultSent = false
            sourcesModel.registry = CoreInterface.singerRegistry
            currentRow = Math.min(Math.max(0, currentRow), sourcesModel.count)
        }
    }
    onClosing: {
        if (!resultSent)
            rejected()
        finished()
    }

    Connections {
        target: dialog.sourcesModel

        function onCountChanged() {
            dialog.currentRow = Math.min(Math.max(0, dialog.currentRow), dialog.sourcesModel.count)
        }

        function onOperationRejected(message) {
            messageBoxHost.MessageBox.warning(qsTr("Cannot update source singers"), message)
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        SplitView {
            Layout.fillWidth: true
            Layout.fillHeight: true

            Rectangle {
                SplitView.preferredWidth: 132
                SplitView.minimumWidth: 104
                SplitView.maximumWidth: 220
                color: Theme.backgroundTertiaryColor

                ListView {
                    id: sourceListView
                    anchors.fill: parent
                    anchors.margins: 8
                    spacing: 8
                    clip: true
                    boundsBehavior: Flickable.StopAtBounds
                    model: dialog.sourcesModel

                    delegate: T.Button {
                        id: sourceButton

                        required property int index
                        required property int singerType
                        required property var singerTree
                        required property string displayName
                        required property bool singerValid
                        required property string warningText
                        required property string effectiveMixGroup
                        required property var modelIndex

                        property int dragIndex: index

                        width: sourceListView.width
                        height: 98
                        highlighted: dialog.currentRow === index
                        Accessible.role: Accessible.ListItem
                        Accessible.name: displayName
                        Drag.active: sourceDragHandler.active
                        Drag.source: sourceButton
                        Drag.hotSpot.x: width / 2
                        Drag.hotSpot.y: height / 2
                        z: sourceDragHandler.active ? 2 : 0
                        onClicked: dialog.currentRow = index

                        contentItem: ColumnLayout {
                            spacing: 4

                            Rectangle {
                                Layout.preferredWidth: 66
                                Layout.preferredHeight: 66
                                Layout.alignment: Qt.AlignHCenter
                                color: Theme.backgroundSecondaryColor
                                border.width: sourceButton.highlighted ? 2 : 1
                                border.color: sourceButton.highlighted ? Theme.accentColor : Theme.borderColor
                                radius: 3

                                SingerAvatar {
                                    anchors.fill: parent
                                    anchors.margins: 2
                                    architectureId: dialog.sourcesModel.architectureId
                                    singerTree: sourceButton.singerTree
                                }

                                IconLabel {
                                    anchors.left: parent.left
                                    anchors.bottom: parent.bottom
                                    width: 18
                                    height: 18
                                    visible: !sourceButton.singerValid
                                    icon.source: "image://fluent-system-icons/warning"
                                    icon.color: Theme.warningColor
                                    icon.width: 18
                                    icon.height: 18
                                    ToolTip.visible: warningHover.hovered
                                    ToolTip.text: sourceButton.warningText

                                    HoverHandler {
                                        id: warningHover
                                    }
                                }

                                ToolButton {
                                    anchors.right: parent.right
                                    anchors.top: parent.top
                                    anchors.margins: 2
                                    width: 22
                                    height: 22
                                    visible: sourceButton.hovered
                                    flat: false
                                    display: AbstractButton.IconOnly
                                    text: qsTr("Remove singer")
                                    icon.source: "image://fluent-system-icons/subtract_circle"
                                    ToolTip.visible: hovered
                                    ToolTip.text: text
                                    onClicked: {
                                        const oldRow = sourceButton.index
                                        if (dialog.sourcesModel.removeSinger(sourceButton.modelIndex))
                                            dialog.currentRow = Math.min(oldRow, Math.max(0, dialog.sourcesModel.count - 1))
                                    }
                                }
                            }

                            Label {
                                Layout.fillWidth: true
                                horizontalAlignment: Text.AlignHCenter
                                elide: Text.ElideRight
                                color: sourceButton.highlighted ? Theme.accentColor : Theme.foregroundPrimaryColor
                                text: sourceButton.displayName
                            }
                        }

                        DragHandler {
                            id: sourceDragHandler
                            target: null
                        }

                        DropArea {
                            anchors.fill: parent
                            onEntered: drag => {
                                if (drag.source && drag.source !== sourceButton)
                                    dialog.moveRootSinger(drag.source.modelIndex, drag.source.index, sourceButton.index)
                            }
                        }

                        TapHandler {
                            acceptedButtons: Qt.RightButton
                            onTapped: {
                                dialog.currentRow = sourceButton.index
                                contextMenu.popup()
                            }
                        }

                        Menu {
                            id: contextMenu

                            Action {
                                text: qsTr("Remove")
                                onTriggered: {
                                    const oldRow = sourceButton.index
                                    if (dialog.sourcesModel.removeSinger(sourceButton.modelIndex))
                                        dialog.currentRow = Math.min(oldRow, Math.max(0, dialog.sourcesModel.count - 1))
                                }
                            }

                            Action {
                                enabled: sourceButton.singerValid && sourceButton.effectiveMixGroup !== ""
                                text: enabled
                                      ? qsTr("Create mixed singer")
                                      : qsTr("Create mixed singer (mixing unavailable)")
                                onTriggered: dialog.sourcesModel.wrapSingerInMixed(sourceButton.modelIndex)
                            }
                        }
                    }

                    footer: T.Button {
                        id: addButton
                        width: sourceListView.width
                        height: 86
                        highlighted: dialog.addPickerSelected
                        enabled: dialog.sourcesModel.canAppend
                        Accessible.role: Accessible.ListItem
                        Accessible.name: qsTr("Add singer")
                        onClicked: dialog.currentRow = dialog.sourcesModel.count

                        contentItem: ColumnLayout {
                            spacing: 4

                            Rectangle {
                                Layout.preferredWidth: 66
                                Layout.preferredHeight: 66
                                Layout.alignment: Qt.AlignHCenter
                                color: Theme.backgroundSecondaryColor
                                border.width: addButton.highlighted ? 2 : 1
                                border.color: addButton.highlighted ? Theme.accentColor : Theme.borderColor
                                radius: 3

                                IconLabel {
                                    anchors.centerIn: parent
                                    icon.source: "image://fluent-system-icons/add"
                                    icon.color: addButton.enabled
                                                ? Theme.foregroundPrimaryColor
                                                : Theme.foregroundDisabledColorChange.apply(Theme.foregroundPrimaryColor)
                                    icon.width: 30
                                    icon.height: 30
                                }
                            }

                            Label {
                                Layout.fillWidth: true
                                horizontalAlignment: Text.AlignHCenter
                                text: qsTr("Add")
                            }
                        }

                        ToolTip.visible: hovered && !enabled
                        ToolTip.text: qsTr("The current source list cannot accept another singer.")
                    }

                    displaced: Transition {
                        NumberAnimation {
                            properties: "y"
                            duration: 120
                            easing.type: Easing.OutCubic
                        }
                    }
                }
            }

            Rectangle {
                SplitView.fillWidth: true
                color: Theme.backgroundPrimaryColor

                StackLayout {
                    anchors.fill: parent
                    currentIndex: dialog.needsInitialPicker || dialog.addPickerSelected ? 0 : 1

                    SingerPicker {
                        architectureId: dialog.sourcesModel.architectureId !== ""
                                        && dialog.sourcesModel.architectureExists
                                        ? dialog.sourcesModel.architectureId
                                        : ""
                        mixGroup: dialog.addPickerSelected
                                  ? dialog.sourcesModel.mixGroup
                                  : ""
                        onSingerSelected: (architectureId, singerId) => dialog.handlePickedSinger(architectureId, singerId)
                    }

                    Loader {
                        active: dialog.selectedModelIndexValid
                        sourceComponent: {
                            dialog.sourcesModel.revision
                            return dialog.selectedSingerType === SourcesPickerModel.MixedSinger
                                   ? mixedEditorComponent
                                   : singleEditorComponent
                        }
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
            height: 60
            color: Theme.backgroundSecondaryColor

            RowLayout {
                anchors.fill: parent
                anchors.margins: 16
                spacing: 12

                ToolButton {
                    visible: dialog.sourcesModel.validationIssues.length > 0
                    flat: true
                    display: AbstractButton.TextBesideIcon
                    text: qsTr("Source singer warnings")
                    icon.source: "image://fluent-system-icons/warning"
                    icon.color: Theme.warningColor
                    onClicked: dialog.showValidationIssues()
                }

                Item {
                    Layout.fillWidth: true
                }

                Button {
                    ThemedItem.controlType: SVS.CT_Accent
                    text: qsTr("OK")
                    onClicked: dialog.acceptDialog()
                }

                Button {
                    text: qsTr("Cancel")
                    onClicked: dialog.rejectDialog()
                }
            }
        }
    }

    Component {
        id: mixedEditorComponent

        MixedSingerEditor {
            sourcesModel: dialog.sourcesModel
            modelIndex: dialog.selectedModelIndex
        }
    }

    Component {
        id: singleEditorComponent

        SourcesSingerControlPanel {
            sourcesModel: dialog.sourcesModel
            modelIndex: dialog.selectedModelIndex
        }
    }
}
