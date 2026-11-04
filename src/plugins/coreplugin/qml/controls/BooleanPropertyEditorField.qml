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
    CheckBox {
        id: checkBox
        readonly property int modelCheckState: d.propertyMapper?.inactive ? Qt.Unchecked : d.value === undefined ? Qt.PartiallyChecked : d.value ? Qt.Checked : Qt.Unchecked
        text: d.label
        tristate: true
        checkState: modelCheckState
        nextCheckState: function() {
            return checkState === Qt.Checked ? Qt.Unchecked : Qt.Checked
        }
        onClicked: {
            if (!d.setValue(checkState === Qt.Checked))
                GlobalHelper.setProperty(checkBox, "checkState", modelCheckState)
        }
    }
}
