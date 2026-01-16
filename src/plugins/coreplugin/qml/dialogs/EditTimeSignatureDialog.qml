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
    property int numerator
    property int denominator
    property int position
    property bool doInsertNew

    title: qsTr("Edit Time Signature")

    onAboutToShow: numeratorSpinBox.forceActiveFocus()

    GridLayout {
        anchors.fill: parent
        columns: 2
        Label {
            id: timeSignatureLabel
            text: qsTr("Time signature")
        }
        RowLayout {
            Layout.fillWidth: true
            SpinBox {
                id: numeratorSpinBox
                Accessible.name: qsTr("Numerator")
                from: 1
                to: 2147483647
                Layout.fillWidth: true
                Layout.horizontalStretchFactor: 1
                value: dialog.numerator
                onValueModified: dialog.numerator = value
            }
            Label {
                text: "/"
            }
            ComboBox {
                id: denominatorComboBox
                Accessible.name: qsTr("Denominator")
                Layout.fillWidth: true
                Layout.horizontalStretchFactor: 1
                model: Array.from({length: 8}, (_, i) => Math.pow(2, i)).map(v => ({
                    text: Qt.locale().toString(v),
                    value: v
                }))
                textRole: "text"
                valueRole: "value"
                currentIndex: indexOfValue(dialog.denominator)
                onActivated: (index) => dialog.denominator = valueAt(index)
            }
        }
        Label {
            id: commonLabel
            text: qsTr("Common")
        }
        RowLayout {
            component CommomTimeSignatureButton: Button {
                required property int numerator
                required property int denominator
                text: Qt.locale().toString(numerator) + "/" + Qt.locale().toString(denominator)
                highlighted: numerator === dialog.numerator && denominator === dialog.denominator
                onClicked: {
                    dialog.numerator = numerator
                    dialog.denominator = denominator
                }
            }
            CommomTimeSignatureButton { numerator: 4; denominator: 4 }
            CommomTimeSignatureButton { numerator: 2; denominator: 4 }
            CommomTimeSignatureButton { numerator: 3; denominator: 4 }
            CommomTimeSignatureButton { numerator: 6; denominator: 8 }
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