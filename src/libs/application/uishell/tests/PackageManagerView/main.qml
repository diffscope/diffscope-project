import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SVSCraft
import SVSCraft.UIComponents

import DiffScope.UIShell

Window {
    id: window
    width: 1080
    height: 600
    visible: true

    required property var model

    PackageManagerView {
        anchors.fill: parent
        model: window.model
        singerExtraDelegate: Rectangle {
            id: singerExtra
            required property var modelData
            required property var index
            height: 160
            color: "red"
            Label {
                anchors.centerIn: parent
                text: "Singer extra fields (customizable by plugins) " + singerExtra.modelData.id
            }
        }
    }
}
