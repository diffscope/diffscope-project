// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SVSCraft
import SVSCraft.UIComponents
import SVSCraft.UIComponents.impl

import DiffScope.UIShell

AbstractPropertyEditorField {
    id: d
    property int from: 0
    property int to: 2147483647
    readonly property MusicTimeSpinBox spinBox: control
    FormGroup {
        Layout.fillWidth: true
        label: d.label
        columnItem: MusicTimeSpinBox {
            id: control
            timeline: d.windowHandle?.projectTimeline.musicTimeline ?? null
            value: d.value ?? 0
            from: d.from
            to: d.to
            contentItem.visible: d.value !== undefined
            onValueModified: () => {
                if (!d.setValue(value, helper.editing))
                    GlobalHelper.setProperty(control, "value", d.value ?? 0)
            }
            SpinBoxPressedHelper {
                id: helper
                spinBox: control
                onPressed: d.beginTransaction()
                onReleased: d.commitTransaction()
            }
        }
    }
}
