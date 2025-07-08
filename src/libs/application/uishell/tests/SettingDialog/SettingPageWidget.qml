import QtQml
import QtQuick
import QtQuick.Layouts

import SVSCraft.UIComponents

Item {
    id: page
    required property string title
    required property QtObject iSettingPage
    property double pageMargins: 0
    anchors.fill: parent
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: page.pageMargins
        anchors.topMargin: 0
        CheckBox {
            text: `${page.title} option`
            onCheckedChanged: page.iSettingPage.markDirty()
        }
        CheckBox {
            text: `Do not accept`
            onCheckedChanged: () => {
                iSettingPage.doNotAccept = checked
                page.iSettingPage.markDirty()
            }
        }
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
        }
    }
}