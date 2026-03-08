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
    FormGroup {
        Layout.fillWidth: true
        label: d.label
        columnItem: TextField {
            text: d.value ?? ""
            onEditingFinished: d.setValue(text)
        }
    }
}
