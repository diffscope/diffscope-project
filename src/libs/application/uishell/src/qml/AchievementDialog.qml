import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Templates as T
import QtQuick.Controls.impl
import QtQuick.Layouts
import QtQml.Models

import SVSCraft
import SVSCraft.UIComponents
import SVSCraft.UIComponents.impl

Window {
    id: window
    width: 400
    height: 600
    readonly property list<QtObject> models: []
    property QtObject currentModel: null
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        RowLayout {
            Layout.fillWidth: true
            Label {
                text: qsTr("Category")
            }
            ComboBox {
                Layout.fillWidth: true
                model: window.models.map(m => m.headerData(0, Qt.Horizontal))
            }
        }
        ScrollView {
            id: achievementsScrollView
            contentWidth: availableWidth
            Layout.fillWidth: true
            Layout.fillHeight: true
            ColumnLayout {
                width: achievementsScrollView.width
                spacing: 0
                Repeater {
                    id: achievementCollectionRepeater
                    model: []
                    delegate: AchievementCard {
                        repeater: achievementCollectionRepeater
                    }
                }
            }
        }
    }
}
