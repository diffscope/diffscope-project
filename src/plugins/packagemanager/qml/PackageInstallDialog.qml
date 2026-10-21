// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl
import QtQuick.Layouts

import SVSCraft
import SVSCraft.UIComponents
import SVSCraft.UIComponents.impl

import DiffScope.PackageManager

Window {
    id: dialog

    required property PackageInstallModel installModel
    property bool completed: false

    signal done(var installed)

    function finish(installed) {
        if (completed)
            return
        completed = true
        done(installed)
        close()
    }

    title: qsTr("Install Packages")
    flags: Qt.Dialog | Qt.CustomizeWindowHint | Qt.WindowTitleHint | Qt.WindowSystemMenuHint | Qt.WindowCloseButtonHint
    modality: Qt.ApplicationModal
    width: 840
    height: 480
    minimumWidth: 600
    minimumHeight: 320

    onClosing: (event) => {
        if (installModel.busy) {
            event.accepted = false
            return
        }
        if (!completed) {
            completed = true
            done(false)
        }
    }

    PackageFileDialog {
        id: packageFileDialog
        onAccepted: dialog.installModel.addFiles(selectedFiles)
    }

    Rectangle {
        anchors.fill: parent
        color: Theme.backgroundPrimaryColor
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Frame {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.margins: 12
            padding: 1

            ColumnLayout {
                anchors.fill: parent
                spacing: 0

                ListView {
                    id: packageList
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    model: dialog.installModel
                    clip: true
                    focus: true
                    activeFocusOnTab: true
                    currentIndex: -1
                    ScrollBar.vertical: ScrollBar {}

                    onCountChanged: {
                        if (count === 0)
                            currentIndex = -1
                        else if (currentIndex < 0)
                            currentIndex = 0
                        else if (currentIndex >= count)
                            currentIndex = count - 1
                    }

                    delegate: ItemDelegate {
                        id: packageDelegate
                        required property var model
                        required property int index
                        readonly property bool error: model.status === PackageInstallModel.InvalidPackage || model.status === PackageInstallModel.BatchConflict
                        readonly property bool warning: model.status === PackageInstallModel.OverwriteRequired
                        readonly property string statusText: {
                            switch (model.status) {
                            case PackageInstallModel.Checking: return qsTr("Checking")
                            case PackageInstallModel.InvalidPackage: return qsTr("Invalid package")
                            case PackageInstallModel.BatchConflict: return qsTr("Batch conflict")
                            case PackageInstallModel.OverwriteRequired: return qsTr("Overwrite required")
                            case PackageInstallModel.AlreadyInstalled: return qsTr("Already installed")
                            default: return ""
                            }
                        }

                        width: ListView.view.width
                        padding: 4
                        leftPadding: 8
                        rightPadding: 8
                        highlighted: ListView.isCurrentItem
                        ThemedItem.flat: true
                        ThemedItem.controlType: highlighted ? SVS.CT_Accent : SVS.CT_Normal
                        Accessible.name: model.filePath + "\n" + model.packageLabel
                        Accessible.description: statusText
                        background: ButtonRectangle {
                            control: packageDelegate
                            checked: packageDelegate.highlighted
                            flat: true
                        }
                        contentItem: RowLayout {
                            spacing: 8

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 1
                                Label {
                                    Layout.fillWidth: true
                                    text: packageDelegate.model.filePath
                                    textFormat: Text.PlainText
                                    font.weight: Font.DemiBold
                                    elide: Text.ElideMiddle
                                }
                                Label {
                                    Layout.fillWidth: true
                                    text: packageDelegate.model.packageLabel
                                    textFormat: Text.PlainText
                                    elide: Text.ElideRight
                                    ThemedItem.foregroundLevel: SVS.FL_Secondary
                                }
                            }

                            RowLayout {
                                visible: packageDelegate.statusText.length > 0
                                spacing: 4
                                IconLabel {
                                    visible: packageDelegate.model.status !== PackageInstallModel.Checking
                                    icon.source: "image://fluent-system-icons/" + (packageDelegate.error ? "dismiss_circle" : packageDelegate.warning ? "warning" : "subtract_circle")
                                    icon.color: packageDelegate.error ? Theme.errorColor : packageDelegate.warning ? Theme.warningColor : Theme.foregroundSecondaryColor
                                    icon.width: 16
                                    icon.height: 16
                                }
                                Label {
                                    text: packageDelegate.statusText
                                    color: packageDelegate.error ? Theme.errorColor : packageDelegate.warning ? Theme.warningColor : Theme.foregroundSecondaryColor
                                }
                            }
                        }
                        onClicked: packageList.currentIndex = index
                    }

                    Label {
                        anchors.centerIn: parent
                        visible: packageList.count === 0
                        text: qsTr("No packages selected")
                        ThemedItem.foregroundLevel: SVS.FL_Secondary
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: 1
                    color: Theme.borderColor
                }

                RowLayout {
                    Layout.fillWidth: true
                    Layout.margins: 4
                    spacing: 2

                    ToolButton {
                        text: qsTr("Add Packages...")
                        icon.source: "image://fluent-system-icons/add"
                        display: AbstractButton.IconOnly
                        focusPolicy: Qt.StrongFocus
                        Accessible.name: text
                        ToolTip.visible: hovered
                        ToolTip.text: text
                        enabled: !dialog.installModel.busy
                        onClicked: packageFileDialog.open()
                    }
                    ToolButton {
                        text: qsTr("Remove from List")
                        icon.source: "image://fluent-system-icons/subtract"
                        display: AbstractButton.IconOnly
                        focusPolicy: Qt.StrongFocus
                        Accessible.name: text
                        ToolTip.visible: hovered
                        ToolTip.text: text
                        enabled: !dialog.installModel.busy && packageList.currentIndex >= 0
                        onClicked: dialog.installModel.removeFile(packageList.currentIndex)
                    }
                    Item { Layout.fillWidth: true }
                }
            }
        }

        Label {
            Layout.fillWidth: true
            Layout.leftMargin: 12
            Layout.rightMargin: 12
            Layout.bottomMargin: visible ? 12 : 0
            visible: text.length > 0
            text: dialog.installModel.errorMessage
            textFormat: Text.PlainText
            color: Theme.errorColor
            wrapMode: Text.Wrap
        }

        Rectangle {
            Layout.fillWidth: true
            implicitHeight: buttons.implicitHeight + 24
            color: Theme.backgroundSecondaryColor

            RowLayout {
                id: buttons
                anchors.fill: parent
                anchors.margins: 12
                BusyIndicator {
                    running: dialog.installModel.busy
                    visible: running
                    implicitWidth: 24
                    implicitHeight: 24
                }
                Item { Layout.fillWidth: true }
                Button {
                    text: qsTr("Install")
                    highlighted: true
                    enabled: dialog.installModel.canInstall
                    onClicked: {
                        if (dialog.installModel.install(dialog))
                            dialog.finish(true)
                    }
                }
                Button {
                    text: qsTranslate("QPlatformTheme", "Cancel")
                    enabled: !dialog.installModel.busy
                    onClicked: dialog.finish(false)
                }
            }
        }
    }
}
