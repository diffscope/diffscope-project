import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SVSCraft
import SVSCraft.UIComponents

import DiffScope.UIShell

AbstractPropertyEditorField {
    id: d
    CheckBox {
        text: d.label
        tristate: true
        checkState: d.propertyMapper?.inactive ? Qt.Unchecked : d.value === undefined ? Qt.PartiallyChecked : d.value ? Qt.Checked : Qt.Unchecked
        nextCheckState: function() {
            return checkState === Qt.Checked ? Qt.Unchecked : Qt.Checked
        }
        onClicked: d.setValue(checkState === Qt.Checked)
    }
}
