import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts

import SVSCraft
import SVSCraft.UIComponents

import DiffScope.UIShell

QtObject {
    id: root
    required property QtObject addOn
    property double now: Date.now()
    property Timer elapsedTimer: Timer {
        interval: 1000
        repeat: true
        running: true
        onTriggered: root.now = Date.now()
    }

    function taskTypeText(type) {
        return [qsTr("Pronunciation"), qsTr("Phoneme"), qsTr("Duration"), qsTr("Parameter"), qsTr("Audio")][type] ?? qsTr("Unknown");
    }

    function taskStateText(state) {
        return [qsTr("Queued"), qsTr("Running"), qsTr("Succeeded"), qsTr("Failed"), qsTr("Canceled")][state] ?? qsTr("Unknown");
    }

    function taskStateIcon(state) {
        return ["clock", "play_circle", "checkmark_circle", "dismiss_circle", "subtract_circle"][state] ?? "question_circle";
    }

    function taskStateColor(state) {
        if (state === 1)
            return Theme.accentColor;
        if (state === 3)
            return Theme.errorColor;
        return Theme.foregroundSecondaryColor;
    }

    function formattedJson(text) {
        if (!text)
            return "";
        try {
            return JSON.stringify(JSON.parse(text), null, 2);
        } catch (error) {
            return text;
        }
    }

    readonly property Component panelComponent: ActionDockingPane {
        id: pane

        function showTaskDiagnostics(task) {
            diagnosticDialog.task = task;
            diagnosticDialog.exchangeIndex = Math.max(0, task.diagnostics.length - 1);
            diagnosticDialog.open();
        }

        FileDialog {
            id: exportDialog
            property var task: null
            title: qsTr("Export Synthesis Diagnostics")
            fileMode: FileDialog.SaveFile
            defaultSuffix: "json"
            nameFilters: [qsTr("JSON files (*.json)"), qsTr("All files (*)")]
            onAccepted: {
                if (!root.addOn.exportDiagnostics(task, selectedFile))
                    pane.MessageBox.critical(qsTr("Export Failed"), qsTr("The synthesis diagnostics could not be exported."));
            }
        }

        Dialog {
            id: diagnosticDialog
            property var task: null
            property int exchangeIndex: 0
            readonly property var exchanges: task?.diagnostics ?? []
            readonly property var exchange: exchanges.length > 0 ? exchanges[Math.min(exchangeIndex, exchanges.length - 1)] : null

            parent: Overlay.overlay
            width: Math.min(900, parent ? parent.width - 48 : 0)
            height: Math.min(720, parent ? parent.height - 48 : 0)
            anchors.centerIn: parent
            modal: true
            title: qsTr("Synthesis Request Diagnostics")
            standardButtons: Dialog.Close
            onClosed: task = null

            contentItem: ColumnLayout {
                spacing: 8

                Label {
                    Layout.fillWidth: true
                    text: diagnosticDialog.task?.errorMessage ?? ""
                    color: Theme.errorColor
                    wrapMode: Text.Wrap
                }

                GridLayout {
                    Layout.fillWidth: true
                    columns: 2
                    columnSpacing: 12
                    rowSpacing: 4

                    Label {
                        text: qsTr("Service Instance ID")
                        ThemedItem.foregroundLevel: SVS.FL_Secondary
                    }
                    Label {
                        Layout.fillWidth: true
                        text: diagnosticDialog.exchange?.serviceInstanceId ?? ""
                        elide: Text.ElideMiddle
                    }
                    Label {
                        text: qsTr("Request ID")
                        ThemedItem.foregroundLevel: SVS.FL_Secondary
                    }
                    Label {
                        Layout.fillWidth: true
                        text: diagnosticDialog.exchange?.requestId ?? ""
                    }
                    Label {
                        text: qsTr("Request URL")
                        ThemedItem.foregroundLevel: SVS.FL_Secondary
                    }
                    TextField {
                        Layout.fillWidth: true
                        readOnly: true
                        selectByMouse: true
                        text: diagnosticDialog.exchange?.url ?? ""
                    }
                    Label {
                        text: qsTr("Status")
                        ThemedItem.foregroundLevel: SVS.FL_Secondary
                    }
                    Label {
                        Layout.fillWidth: true
                        text: diagnosticDialog.exchange?.statusCode > 0
                              ? qsTr("HTTP %1").arg(diagnosticDialog.exchange.statusCode)
                              : qsTr("No HTTP response")
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    visible: diagnosticDialog.exchanges.length > 1

                    Label {
                        Layout.fillWidth: true
                        text: qsTr("Request")
                    }
                    SpinBox {
                        from: 1
                        to: Math.max(1, diagnosticDialog.exchanges.length)
                        value: diagnosticDialog.exchangeIndex + 1
                        editable: false
                        onValueModified: diagnosticDialog.exchangeIndex = value - 1
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    spacing: 8

                    GroupBox {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        title: qsTr("Request")

                        ColumnLayout {
                            anchors.fill: parent

                            RowLayout {
                                Layout.fillWidth: true
                                Label {
                                    Layout.fillWidth: true
                                    text: diagnosticDialog.exchange
                                          ? "%1 · %2".arg(diagnosticDialog.exchange.method).arg(qsTr("Attempt %L1").arg(diagnosticDialog.exchange.attempt))
                                          : ""
                                    ThemedItem.foregroundLevel: SVS.FL_Secondary
                                }
                                Button {
                                    text: qsTr("Copy Request")
                                    enabled: Boolean(diagnosticDialog.exchange?.requestBody)
                                    onClicked: root.addOn.copyDiagnosticRequest(diagnosticDialog.task, diagnosticDialog.exchangeIndex)
                                }
                            }
                            ScrollView {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                TextArea {
                                    readOnly: true
                                    selectByMouse: true
                                    wrapMode: TextEdit.NoWrap
                                    text: root.formattedJson(diagnosticDialog.exchange?.requestBody ?? "")
                                    placeholderText: qsTr("This request has no body.")
                                }
                            }
                        }
                    }

                    GroupBox {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        title: qsTr("Response")

                        ColumnLayout {
                            anchors.fill: parent

                            RowLayout {
                                Layout.fillWidth: true
                                Label {
                                    Layout.fillWidth: true
                                    text: diagnosticDialog.exchange?.errorMessage ?? ""
                                    color: Theme.errorColor
                                    elide: Text.ElideRight
                                    ToolTip.visible: responseErrorHoverHandler.hovered && truncated
                                    ToolTip.text: text
                                    HoverHandler { id: responseErrorHoverHandler }
                                }
                                Button {
                                    text: qsTr("Copy Response")
                                    enabled: Boolean(diagnosticDialog.exchange?.responseBody)
                                    onClicked: root.addOn.copyDiagnosticResponse(diagnosticDialog.task, diagnosticDialog.exchangeIndex)
                                }
                            }
                            ScrollView {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                TextArea {
                                    readOnly: true
                                    selectByMouse: true
                                    wrapMode: TextEdit.NoWrap
                                    text: root.formattedJson(diagnosticDialog.exchange?.responseBody ?? "")
                                    placeholderText: qsTr("The service returned no response body.")
                                }
                            }
                        }
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    Item { Layout.fillWidth: true }
                    Button {
                        text: qsTr("Export Diagnostics")
                        enabled: Boolean(diagnosticDialog.task?.diagnosticFilePath)
                        onClicked: {
                            exportDialog.task = diagnosticDialog.task;
                            exportDialog.open();
                        }
                    }
                }
            }
        }

        header: ToolBarContainer {
            anchors.fill: parent
            ToolBarContainerStretch {}
            ToolButton {
                text: qsTr("Open Diagnostics Folder")
                display: AbstractButton.IconOnly
                icon.source: "image://fluent-system-icons/folder_open"
                ToolTip.visible: hovered
                ToolTip.text: text
                onClicked: Qt.openUrlExternally(root.addOn.diagnosticsDirectoryUrl)
            }
            ToolButton {
                text: qsTr("Clear Diagnostics")
                display: AbstractButton.IconOnly
                icon.source: "image://fluent-system-icons/delete"
                ToolTip.visible: hovered
                ToolTip.text: text
                onClicked: {
                    if (pane.MessageBox.question(qsTr("Clear Diagnostics"), qsTr("Delete all saved synthesis diagnostics?")) === SVS.Yes)
                        root.addOn.clearDiagnostics();
                }
            }
            ToolButton {
                text: root.addOn?.refreshing ? qsTr("Refreshing...") : qsTr("Refresh")
                display: AbstractButton.IconOnly
                icon.source: "image://fluent-system-icons/arrow_sync"
                onClicked: root.addOn.refreshAll()
            }
        }

        ScrollView {
            anchors.fill: parent
            contentWidth: availableWidth

            ColumnLayout {
                width: parent.width
                spacing: 8

                Repeater {
                    id: serviceRepeater
                    model: root.addOn?.serviceModel ?? null

                    delegate: Frame {
                        id: serviceDelegate
                        required property var model
                        required property int index
                        property bool expanded: false
                        readonly property int taskCount: model.tasks.length
                        readonly property bool hasTasks: taskCount > 0
                        readonly property color statusColor: model.healthStatus === 4 ? Theme.errorColor : model.healthStatus === 3 ? Theme.accentColor : model.healthStatus === 2 ? Theme.warningColor : Theme.foregroundSecondaryColor
                        readonly property string statusDetails: {
                            let lines = [];
                            if (model.lastHealthCheck)
                                lines.push(qsTr("Last checked: %1").arg(model.lastHealthCheck));
                            if (model.errorMessage)
                                lines.push(model.errorMessage);
                            return lines.join("\n");
                        }

                        Layout.fillWidth: true
                        Layout.leftMargin: 8
                        Layout.rightMargin: 8
                        Layout.topMargin: index === 0 ? 8 : 0
                        Layout.bottomMargin: index === serviceRepeater.count - 1 ? 8 : 0
                        padding: 0
                        clip: true
                        Accessible.role: Accessible.ListItem
                        Accessible.name: model.name

                        background: Rectangle {
                            color: Theme.backgroundSecondaryColor
                            border.width: 1
                            border.color: Theme.borderColor
                            radius: 4
                        }

                        contentItem: ColumnLayout {
                            spacing: 0

                            ColumnLayout {
                                Layout.fillWidth: true
                                Layout.margins: 12
                                spacing: 6

                                RowLayout {
                                    spacing: 8

                                    IconLabel {
                                        Layout.preferredWidth: 24
                                        Layout.preferredHeight: 24
                                        icon.source: "image://fluent-system-icons/cloud?size=24&style=regular"
                                        icon.color: Theme.foregroundSecondaryColor
                                        icon.width: 24
                                        icon.height: 24
                                    }

                                    Label {
                                        Layout.fillWidth: true
                                        text: serviceDelegate.model.name
                                        font.weight: Font.DemiBold
                                        elide: Text.ElideRight
                                    }

                                    ToolButton {
                                        Layout.preferredWidth: 28
                                        Layout.preferredHeight: 28
                                        text: serviceDelegate.expanded ? qsTr("Hide Tasks") : qsTr("Show Tasks")
                                        display: AbstractButton.IconOnly
                                        icon.source: serviceDelegate.expanded ? "image://fluent-system-icons/chevron_up" : "image://fluent-system-icons/chevron_down"
                                        ToolTip.visible: hovered
                                        ToolTip.text: text
                                        onClicked: serviceDelegate.expanded = !serviceDelegate.expanded
                                    }
                                }

                                RowLayout {
                                    spacing: 6

                                    RowLayout {
                                        spacing: 4

                                        IconLabel {
                                            icon.source: `image://fluent-system-icons/${serviceDelegate.model.healthIcon}`
                                            icon.color: serviceDelegate.statusColor
                                        }

                                        Label {
                                            text: serviceDelegate.model.healthText
                                            color: serviceDelegate.statusColor
                                        }

                                        ToolTip.visible: healthStatusHoverHandler.hovered && serviceDelegate.statusDetails.length > 0
                                        ToolTip.text: serviceDelegate.statusDetails

                                        HoverHandler {
                                            id: healthStatusHoverHandler
                                        }
                                    }

                                    Label {
                                        Layout.fillWidth: true
                                        Layout.minimumWidth: 0
                                        text: qsTr("%L1 running, %L2 queued").arg(serviceDelegate.model.runningTaskCount).arg(serviceDelegate.model.queuedTaskCount)
                                        elide: Text.ElideRight
                                        horizontalAlignment: Text.AlignRight
                                        ThemedItem.foregroundLevel: SVS.FL_Secondary
                                    }
                                }

                                RowLayout {
                                    spacing: 6

                                    IconLabel {
                                        icon.source: "image://fluent-system-icons/link"
                                        icon.color: Theme.foregroundSecondaryColor
                                    }

                                    Label {
                                        Layout.fillWidth: true
                                        text: serviceDelegate.model.baseUrl
                                        elide: Text.ElideMiddle
                                        ThemedItem.foregroundLevel: SVS.FL_Secondary
                                        ToolTip.visible: urlHoverHandler.hovered && truncated
                                        ToolTip.text: text

                                        HoverHandler {
                                            id: urlHoverHandler
                                        }
                                    }
                                }
                            }

                            Rectangle {
                                visible: serviceDelegate.expanded && serviceDelegate.hasTasks
                                Layout.fillWidth: true
                                Layout.preferredHeight: 1
                                color: Theme.borderColor
                            }

                            ColumnLayout {
                                visible: serviceDelegate.expanded && serviceDelegate.hasTasks
                                Layout.fillWidth: true
                                Layout.leftMargin: 12
                                Layout.rightMargin: 12
                                Layout.topMargin: 8
                                Layout.bottomMargin: 12
                                spacing: 4

                                Repeater {
                                    model: serviceDelegate.model.tasks

                                    delegate: Rectangle {
                                        id: taskDelegate
                                        required property var modelData

                                        Layout.fillWidth: true
                                        Layout.preferredHeight: taskLayout.implicitHeight + 8
                                        color: Theme.backgroundTertiaryColor
                                        radius: 3
                                        Accessible.role: Accessible.ListItem
                                        Accessible.name: "%1, %2".arg(root.taskTypeText(modelData.type)).arg(root.taskStateText(modelData.state))

                                        ToolTip.visible: taskHoverHandler.hovered && modelData.state === 3 && Boolean(modelData.errorMessage)
                                        ToolTip.text: modelData.errorMessage

                                        HoverHandler {
                                            id: taskHoverHandler
                                        }

                                        RowLayout {
                                            id: taskLayout
                                            anchors.fill: parent
                                            anchors.leftMargin: 8
                                            anchors.rightMargin: 8
                                            spacing: 8

                                            IconLabel {
                                                icon.source: `image://fluent-system-icons/${root.taskStateIcon(taskDelegate.modelData.state)}`
                                                icon.color: root.taskStateColor(taskDelegate.modelData.state)
                                            }

                                            Label {
                                                Layout.fillWidth: true
                                                text: root.taskTypeText(taskDelegate.modelData.type)
                                                elide: Text.ElideRight
                                            }

                                            Label {
                                                text: root.taskStateText(taskDelegate.modelData.state)
                                                color: root.taskStateColor(taskDelegate.modelData.state)
                                            }

                                            Label {
                                                visible: taskDelegate.modelData.state === 1 && Boolean(taskDelegate.modelData.startedAt)
                                                text: qsTr("%L1 s").arg(Math.max(0, Math.floor((root.now - taskDelegate.modelData.startedAt.getTime()) / 1000)))
                                                ThemedItem.foregroundLevel: SVS.FL_Secondary
                                            }

                                            ToolButton {
                                                visible: taskDelegate.modelData.state === 3 && taskDelegate.modelData.diagnostics.length > 0
                                                text: qsTr("View Diagnostics")
                                                display: AbstractButton.IconOnly
                                                icon.source: "image://fluent-system-icons/document_search"
                                                ToolTip.visible: hovered
                                                ToolTip.text: text
                                                onClicked: pane.showTaskDiagnostics(taskDelegate.modelData)
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                Label {
                    Layout.fillWidth: true
                    Layout.margins: 16
                    visible: serviceRepeater.count === 0
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode: Text.Wrap
                    text: qsTr("No synthesis services configured. Add one in Settings > Synthesis > Services.")
                    ThemedItem.foregroundLevel: SVS.FL_Secondary
                }
            }
        }
    }
}
