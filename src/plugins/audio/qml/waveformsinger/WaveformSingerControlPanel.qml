import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import DiffScope.Audio

Item {
    id: control

    readonly property var singerTypes: WaveformSingerTypeCatalog.entries

    signal extraEdited()

    function setSingerId(singerId) {
    }

    function setExtra(extra) {
        let typeIndex = -1
        if (extra !== null && typeof extra === "object" && !Array.isArray(extra)
                && Object.prototype.hasOwnProperty.call(extra, "type")) {
            typeIndex = singerTypes.findIndex(item => item.type === extra.type)
        }

        const extraValid = typeIndex >= 0
        typeComboBox.currentIndex = extraValid ? typeIndex : 0
        if (!extraValid)
            extraEdited()
    }

    function getExtra() {
        const typeIndex = typeComboBox.currentIndex >= 0 ? typeComboBox.currentIndex : 0
        return { type: singerTypes[typeIndex].type }
    }

    RowLayout {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: 16

        Label {
            text: qsTr("Type")
        }

        ComboBox {
            id: typeComboBox

            Layout.fillWidth: true
            model: control.singerTypes
            textRole: "name"
            valueRole: "type"
            onActivated: control.extraEdited()
        }
    }
}
