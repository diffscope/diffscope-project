// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SVSCraft.UIComponents

Dialog {
    id: dialog
    required property QtObject addOn

    title: qsTr("New Panels")
    modal: false
    standardButtons: Dialog.Close
    width: 420
    property var entries: []
    onAboutToShow: {
        if (!dialog.addOn?.session || dialog.addOn.session.shuttingDown)
            return
        entries = dialog.addOn.newPanelEntries
        dialog.addOn.markNewPanelsPresented()
    }
    contentItem: ColumnLayout {
        Label {
            text: qsTr("Drag a panel to the workspace to place it.")
            wrapMode: Text.Wrap
            Layout.fillWidth: true
        }

        Repeater {
            model: dialog.entries
            delegate: ItemDelegate {
                id: entry
                required property var modelData
                readonly property DockingPanel panel: dialog.addOn?.session?.context?.panel(modelData.id) ?? null
                text: modelData.text
                icon.source: modelData.iconSource

                Binding {
                    target: entry
                    property: "icon.color"
                    value: entry.modelData.iconColor
                    when: entry.modelData.iconColor.valid
                }
                Layout.fillWidth: true
                enabled: Boolean(dialog.addOn?.session) && !dialog.addOn.session.shuttingDown && !dialog.addOn.session.busy
                onClicked: if (panel?.group)
                    dialog.addOn.session.context.show(panel)
                DragHandler {
                    enabled: Boolean(entry.panel) && !entry.panel.group
                    target: null
                    onActiveChanged: if (active)
                        dialog.addOn.session.context.startDrag(entry.panel)
                }
            }
        }
    }
}
