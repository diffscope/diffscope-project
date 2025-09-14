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
import SVSCraft.Extras

Window {
    id: window
    width: 600
    height: 600
    title: qsTr("Achievements")
    color: Theme.backgroundPrimaryColor
    property QtObject settings: null
    property string settingCategory: ""
    readonly property list<QtObject> models: []
    property list<AchievementProxyModel> proxyModels: []
    property alias showNotification: showNotificationCheckBox.checked

    Settings {
        id: s
        settings: window.settings
        category: window.settingCategory
        property alias _showNotification: window.showNotification
    }

    Component {
        id: achievementProxyModelComponent
        AchievementProxyModel {
        }
    }

    Window {
        id: notificationWindow
        x: 64
        y: 64
        width: 400
        height: 104
        flags: Qt.Tool
        property string name
        property url icon
        property var iconColor
        color: Theme.backgroundTertiaryColor
        title: qsTr("Achievement Completed - %1").arg(Application.name)
        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 4
            spacing: 4
            Card {
                Layout.fillWidth: parent
                implicitHeight: 60
                Component.onCompleted: () => {
                    background.color = Theme.backgroundTertiaryColor
                }
                title: Label {
                    text: qsTr("Achievement Completed")
                    font.weight: Font.DemiBold
                    color: Theme.accentColor
                }
                subtitle: Label {
                    text: notificationWindow.name
                    color: Theme.foregroundPrimaryColor
                }
                image: Rectangle {
                    color: Theme.backgroundQuaternaryColor
                    IconLabel {
                        anchors.centerIn: parent
                        icon.source: notificationWindow.icon
                        icon.color: notificationWindow.iconColor?.valid ? notificationWindow.iconColor : Theme.foregroundSecondaryColor
                        icon.width: 32
                        icon.height: 32
                    }
                }
                toolBar: RowLayout {
                    IconLabel {
                        icon.source: "qrc:/qt/qml/DiffScope/UIShell/assets/CheckmarkCircle16Filled.svg"
                        icon.color: Theme.accentColor
                    }
                }
            }
            RowLayout {
                Layout.fillWidth: true
                Item {
                    Layout.fillWidth: true
                }
                Button {
                    ThemedItem.controlType: SVS.CT_Accent
                    text: qsTr("View Achievements")
                    onClicked: () => {
                        notificationWindow.close()
                        window.show()
                        if (window.visibility === Window.Minimized) {
                            window.showNormal()
                        }
                        window.raise()
                        window.requestActivate()
                    }
                }
            }
        }

        Timer {
            id: timer
            interval: 5000
            onTriggered: notificationWindow.close()
        }

        function popup() {
            show()
            timer.restart()
        }
    }

    Component.onCompleted: () => {
        for (let sourceModel of models) {
            let proxyModel = achievementProxyModelComponent.createObject(this, {sourceModel})
            proxyModels.push(proxyModel)
            for (let i = 0; i < sourceModel.rowCount(); i++) {
                let idx = sourceModel.index(i, 0)
                let id = idx.data(USDef.AR_IdRole)
                if (s.value(id)) {
                    proxyModel.handleAchievementCompleted(id, true)
                }
            }
            sourceModel.achievementCompleted.connect(id => {
                if (!proxyModel.handleAchievementCompleted(id, true)) {
                    return
                }
                s.setValue(id, true)
                if (window.showNotification) {
                    let idx = proxyModel.indexForId(id)
                    notificationWindow.name = idx.data(USDef.AR_NameRole)
                    notificationWindow.icon = idx.data(USDef.AR_IconRole)
                    notificationWindow.iconColor = idx.data(USDef.AR_IconColorRole)
                    notificationWindow.popup()
                }
            })
        }
    }

    component AchievementCard: T.Button {
        id: achievementCard
        required property var modelData
        required property int index
        required property Repeater repeater
        Accessible.role: Accessible.ListItem
        Layout.fillWidth: true
        height: 60
        readonly property bool achievementVisible: !modelData.hidden || completed
        readonly property bool completed: modelData.completed
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
                opacity: achievementCard.achievementVisible ? 1 : 0
                text: achievementCard.modelData.name
                font.weight: Font.DemiBold
                color: achievementCard.completed ? Theme.accentColor : Theme.foregroundSecondaryColor
            }
            subtitle: Label {
                opacity: achievementCard.achievementVisible ? 1 : 0
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
        }
        Label {
            anchors.centerIn: parent
            visible: !achievementCard.achievementVisible
            text: qsTr("Hidden achievement")
        }
        Menu {
            id: contextMenu
            Action {
                text: qsTr("Reset to Incomplete")
                enabled: achievementCard.completed
                onTriggered: () => {
                    let proxyModel = achievementCard.repeater.model
                    if (!proxyModel.handleAchievementCompleted(achievementCard.modelData.id, false)) {
                        return
                    }
                    s.setValue(achievementCard.modelData.id, false)
                }
            }
        }
        Keys.onMenuPressed: contextMenu.popup()
        TapHandler {
            acceptedButtons: Qt.RightButton
            onSingleTapped: contextMenu.popup()
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
            CheckBox {
                id: showNotificationCheckBox
                text: qsTr('Show "Achievement Completed" notification')
                checked: true
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
                    model: window.proxyModels[comboBox.currentIndex] ?? null
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
