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
    width: 600
    height: 600
    title: qsTr("Achievements")
    color: Theme.backgroundPrimaryColor
    readonly property list<QtObject> models: []

    component AchievementCard: T.Button {
        id: achievementCard
        required property var modelData
        required property int index
        required property Repeater repeater
        Accessible.role: Accessible.ListItem
        Layout.fillWidth: true
        height: 60
        readonly property bool achievementVisible: !modelData.hidden || completed
        readonly property bool completed: false
        Card {
            anchors.fill: parent
            atTop: achievementCard.index === 0
            atBottom: achievementCard.index === achievementCard.repeater.count - 1
            property color _baseColor: Theme.backgroundTertiaryColor
            property color color: achievementCard.pressed ? Theme.controlPressedColorChange.apply(_baseColor) : achievementCard.hovered ? Theme.controlHoveredColorChange.apply(_baseColor) : _baseColor

            Behavior on color {
                ColorAnimation {
                    duration: Theme.colorAnimationDuration
                    easing.type: Easing.OutCubic
                }
            }
            onColorChanged: background.color = color
            title: Label {
                visible: achievementCard.achievementVisible
                text: achievementCard.modelData.name
                font.weight: Font.DemiBold
                color: achievementCard.completed ? Theme.accentColor : Theme.foregroundSecondaryColor
            }
            subtitle: Label {
                visible: achievementCard.achievementVisible
                text: achievementCard.modelData.description
                color: achievementCard.completed ? Theme.foregroundPrimaryColor : Theme.foregroundSecondaryColor
            }
            image: Rectangle {
                visible: achievementCard.achievementVisible
                color: Theme.backgroundQuaternaryColor
                IconLabel {
                    anchors.centerIn: parent
                    icon.source: achievementCard.modelData.icon
                    icon.color: achievementCard.modelData.iconColor?.valid ? achievementCard.modelData.iconColor : Theme.foregroundSecondaryColor
                    icon.width: 32
                    icon.height: 32
                }
            }
            toolBar: RowLayout {
                IconLabel {
                    visible: achievementCard.completed
                    icon.source: "qrc:/qt/qml/DiffScope/UIShell/assets/CheckmarkCircle16Filled.svg"
                    icon.color: Theme.accentColor
                }
            }
            Label {
                anchors.centerIn: parent
                visible: !achievementCard.achievementVisible
                text: qsTr("Hidden achievement")
            }
        }
    }
    
    ColumnLayout {
        anchors.fill: parent
        spacing: 0
        RowLayout {
            Layout.fillWidth: true
            Layout.margins: 12
            Label {
                text: qsTr("Category")
            }
            ComboBox {
                id: comboBox
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
                x: 12
                width: achievementsScrollView.width - 24
                spacing: 0
                Repeater {
                    id: achievementCollectionRepeater
                    model: AchievementProxyModel {
                        sourceModel: window.models[comboBox.currentIndex] ?? null
                    }
                    delegate: AchievementCard {
                        repeater: achievementCollectionRepeater
                    }
                }
                Item {
                    height: 12
                }
            }
        }
    }
}
