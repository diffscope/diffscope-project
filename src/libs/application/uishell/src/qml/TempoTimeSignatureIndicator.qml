import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SVSCraft
import SVSCraft.UIComponents

Item {
    id: control
    property bool backgroundVisible: true
    property string tempoText
    property string timeSignatureText

    implicitWidth: layout.implicitWidth
    implicitHeight: 24

    signal tempoClicked()
    signal timeSignatureClicked()

    Rectangle {
        id: background
        anchors.fill: parent
        visible: control.backgroundVisible
        color: Theme.backgroundTertiaryColor
        border.color: Theme.borderColor
        radius: 4
    }

    RowLayout {
        id: layout
        anchors.fill: parent
        spacing: 0
        component IndicatorToolButton: ToolButton {
            font.family: Theme.font.family
            font.pixelSize: 16
            implicitWidth: Math.max(control.backgroundVisible ? 54 : 0, implicitContentWidth + 16)
            implicitHeight: 24
            leftPadding: 12
            rightPadding: 12
        }
        IndicatorToolButton {
            text: control.tempoText
            Accessible.description: qsTr("Tempo")
            onClicked: control.tempoClicked()

        }
        IndicatorToolButton {
            text: control.timeSignatureText
            Accessible.description: qsTr("Time Signature")
            onClicked: control.timeSignatureClicked()
        }
    }
}