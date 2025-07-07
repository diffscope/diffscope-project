import QtQml
import QtQuick

import SVSCraft.UIComponents

Rectangle {
    id: page
    required property string title
    property double pageMargins: 0
    anchors.fill: parent
    color: Theme.errorColor
    Rectangle {
        anchors.fill: parent
        anchors.margins: page.pageMargins
        anchors.topMargin: 0
        color: Theme.accentColor
        Label {
            anchors.centerIn: parent
            text: page.title
        }
    }
}