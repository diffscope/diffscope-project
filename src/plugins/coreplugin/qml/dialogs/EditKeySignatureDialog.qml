import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SVSCraft
import SVSCraft.UIComponents

import DiffScope.UIShell
import DiffScope.Core

Dialog {
    id: dialog

    property MusicTimeline timeline
    property int tonality
    property int mode
    property int accidentalType
    property int position
    property bool doInsertNew

    title: qsTr("Edit Key Signature")

    onAboutToShow: tonalityComboBox.forceActiveFocus()

    GridLayout {
        anchors.fill: parent
        columns: 2
        // TODO
        SpinBox {
            Layout.columnSpan: 2
            Layout.fillWidth: true
            from: 0
            to: 4095
            value: dialog.mode
            onValueModified: dialog.mode = value
        }
        Label {
            id: tonalityLabel
            text: qsTr("Tonality")
        }
        ComboBox {
            id: tonalityComboBox
            Accessible.labelledBy: tonalityLabel
            Accessible.name: tonalityLabel.text
            Layout.fillWidth: true
            model: ["C", "C\u266f/D\u266d", "D", "D\u266f/E\u266d", "E", "F", "F\u266f/G\u266d", "G", "G\u266f/A\u266d", "A", "A\u266f/B\u266d", "B"]
            enabled: dialog.mode !== 0
            currentIndex: dialog.tonality
            onActivated: (index) => dialog.tonality = index
        }
        Label {
            id: modeLabel
            text: qsTr("Mode")
        }
        ComboBox {
            id: modeComboBox
            Accessible.labelledBy: modeLabel
            Accessible.name: qsTr("Mode")
            Layout.fillWidth: true
            textRole: "name"
            valueRole: "musicMode"
            model: SVS.getBuiltInMusicModeInfoList()
            currentValue: dialog.mode
            onActivated: (index) => dialog.mode = valueAt(index)
        }
        Label {
            id: accidentalTypeLabel
            text: qsTr("Accidental Type")
        }
        TabBar {
            Layout.fillWidth: true
            currentIndex: dialog.accidentalType
            TabButton {
                icon.source: "qrc:/diffscope/coreplugin/icons/accidental_flat.svg"
                text: qsTr("Flat")
                onClicked: dialog.accidentalType = 0
            }
            TabButton {
                icon.source: "qrc:/diffscope/coreplugin/icons/accidental_sharp.svg"
                text: qsTr("Sharp")
                onClicked: dialog.accidentalType = 1
            }
        }
        Label {
            id: positionLabel
            text: qsTr("Position")
        }
        MusicTimeSpinBox {
            id: positionSpinBox
            Accessible.labelledBy: positionLabel
            Accessible.name: positionLabel.text
            Layout.fillWidth: true
            timeline: dialog.timeline
            value: dialog.position
            onValueModified: dialog.position = value
        }
        RowLayout {
            Layout.columnSpan: 2
            RadioButton {
                text: qsTr("Modify existing one")
                checked: !dialog.doInsertNew
                onClicked: dialog.doInsertNew = false
            }
            RadioButton {
                text: qsTr("Insert new one")
                checked: dialog.doInsertNew
                onClicked: dialog.doInsertNew = true
            }
        }
    }
    standardButtons: DialogButtonBox.Ok
}