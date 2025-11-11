import QtQml
import QtQuick
import QtQuick.Layouts
import QtQuick.Effects
import QtQuick.Controls
import QtQuick.Controls.impl

import SVSCraft
import SVSCraft.UIComponents

import DiffScope.UIShell

Item {
    id: dialog
    required property QtObject handle
    property bool popupLike: false
    readonly property bool hasProgress: handle?.hasProgress ?? false
    implicitWidth: 360
    implicitHeight: bubbleLayout.implicitHeight + 24
    MouseArea {
        acceptedButtons: Qt.AllButtons
        anchors.fill: parent
    }
    Rectangle {
        id: backgroundArea
        anchors.fill: parent
        color: Theme.backgroundTertiaryColor
        border.color: dialog.handle?.icon === SVS.Warning ? Theme.warningColor : dialog.handle?.icon === SVS.Critical ? Theme.errorColor : dialog.handle?.icon === SVS.Success || dialog.handle?.icon === SVS.Tip ? Theme.accentColor : Theme.borderColor
        border.width: 2
        radius: 4
    }
    MultiEffect {
        source: backgroundArea
        anchors.fill: parent
        shadowEnabled: dialog.popupLike
        shadowColor: Theme.shadowColor
    }
    ColumnLayout {
        id: bubbleLayout
        anchors.fill: parent
        anchors.leftMargin: 12
        anchors.rightMargin: 12
        anchors.topMargin: 8
        anchors.bottomMargin: 8
        spacing: 8
        RowLayout {
            Layout.fillWidth: true
            spacing: 6
            ColorImage {
                visible: dialog.handle?.icon === SVS.Information || dialog.handle?.icon === SVS.Warning || dialog.handle?.icon === SVS.Critical || dialog.handle?.icon === SVS.Question || dialog.handle?.icon === SVS.Success || dialog.handle?.icon === SVS.Tip
                Layout.alignment: Qt.AlignTop
                Layout.preferredWidth: 24
                Layout.preferredHeight: 24
                color: dialog.handle?.icon === SVS.Warning ? Theme.warningColor : dialog.handle?.icon === SVS.Critical ? Theme.errorColor : dialog.handle?.icon === SVS.Success || dialog.handle?.icon === SVS.Tip ? Theme.accentColor : Theme.foregroundPrimaryColor
                source: `image://fluent-system-icons/${dialog.handle?.icon === SVS.Information ? "info" : dialog.handle?.icon === SVS.Warning ? "warning" : dialog.handle?.icon === SVS.Critical ? "dismiss_circle" : dialog.handle?.icon === SVS.Question ? "question_circle" : dialog.handle?.icon === SVS.Success ? "checkmark_circle" : "info_sparkle"}?size=24&style=regular`
                sourceSize.width: 24
                sourceSize.height: 24
            }
            Label {
                Layout.alignment: Qt.AlignTop
                Layout.topMargin: 4
                Layout.fillWidth: true
                text: dialog.handle?.title ?? ""
                textFormat: dialog.handle?.textFormat ?? Text.PlainText
                onLinkActivated: (link) => dialog.handle?.linkActivated(link)
                wrapMode: Text.Wrap
            }
            Button {
                Layout.alignment: Qt.AlignTop
                flat: true
                visible: dialog.handle?.permanentlyHideable ?? false
                icon.source: "image://fluent-system-icons/eye_off"
                display: AbstractButton.IconOnly
                text: qsTr("Do Not Show Again")
                padding: 2
                leftPadding: 2
                rightPadding: 2
                background.implicitWidth: 20
                background.implicitHeight: 20
                onClicked: dialog.handle.permanentlyHideClicked()
            }
            Button {
                Layout.alignment: Qt.AlignTop
                flat: true
                visible: dialog.popupLike
                icon.source: "image://fluent-system-icons/chevron_down"
                display: AbstractButton.IconOnly
                text: qsTr("Collapse to Notifications Panel")
                padding: 2
                leftPadding: 2
                rightPadding: 2
                background.implicitWidth: 20
                background.implicitHeight: 20
                onClicked: dialog.handle.hideClicked()
            }
            Button {
                Layout.alignment: Qt.AlignTop
                flat: true
                visible: dialog.handle?.closable ?? false
                icon.source: "image://fluent-system-icons/dismiss?size=12"
                display: AbstractButton.IconOnly
                text: qsTr("Clear")
                icon.width: 14
                icon.height: 14
                padding: 2
                leftPadding: 2
                rightPadding: 2
                background.implicitWidth: 20
                background.implicitHeight: 20
                onClicked: dialog.handle.closeClicked()
            }
        }
        Label {
            ThemedItem.foregroundLevel: SVS.FL_Secondary
            visible: text.length !== 0
            text: dialog.handle?.text ?? ""
            textFormat: dialog.handle?.textFormat ?? Text.PlainText
            Layout.fillWidth: true
            wrapMode: Text.Wrap
            onLinkActivated: (link) => dialog.handle.linkActivated(link)
        }
        RowLayout {
            spacing: 2
            Layout.fillWidth: true
            visible: dialog.handle?.hasProgress ?? false
            ProgressBar {
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignVCenter
                indeterminate: (dialog.handle?.progress ?? 0) < 0 || (dialog.handle?.progress ?? 0) > 1
                from: 0
                to: 1
                value: dialog.handle?.progress ?? 0
            }
            Button {
                Layout.alignment: Qt.AlignVCenter
                visible: dialog.handle?.progressAbortable ?? false
                background: Item {}
                display: AbstractButton.IconOnly
                text: qsTr("Abort")
                icon.source: "image://fluent-system-icons/dismiss_circle?size=12"
                icon.height: 12
                icon.width: 12
                padding: 0
                leftPadding: 0
                rightPadding: 0
                onClicked: dialog.handle.abortClicked()
            }
        }
        RowLayout {
            spacing: 8
            Layout.alignment: Qt.AlignRight
            visible: (dialog.handle?.buttons ?? []).length !== 0
            Repeater {
                model: dialog.handle?.buttons ?? []
                delegate: Button {
                    required property string modelData
                    required property int index
                    text: modelData
                    ThemedItem.controlType: index === (dialog.handle?.primaryButton ?? -1) ? SVS.CT_Accent : SVS.CT_Normal
                    onClicked: dialog.handle.buttonClicked(index)
                }
            }
        }
    }
    HoverHandler {
        target: dialog
        onHoveredChanged: () => {
            if (!hovered) {
                dialog.handle.hoverExited()
            } else {
                dialog.handle.hoverEntered()
            }
        }
    }
}