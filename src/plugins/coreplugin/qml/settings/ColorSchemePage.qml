import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQml.Models

import SVSCraft
import SVSCraft.UIComponents

Item {
    id: page
    required property QtObject pageHandle
    property bool started: false

    required property QtObject collection

    readonly property TextMatcher matcher: TextMatcher {}

    anchors.fill: parent

    Connections {
        target: page.collection
        function onUnsavedPresetUpdated() {
            page.pageHandle.markDirty()
        }
    }

    component ColorPropertyEditor: Pane {
        ColumnLayout {
            spacing: 32
            width: parent.width
            RowLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Label {
                    text: qsTr("Color")
                }
                ColorComboBox {
                    id: colorComboBox
                    Layout.fillWidth: true
                    readonly property var modelData: colorSchemePropertiesModel.get(listView.currentIndex)
                    function updateColor() {
                        if (!modelData || modelData.colorChange)
                            return
                        color = page.collection.value(modelData.name)
                    }
                    onModelDataChanged: updateColor();
                    Connections {
                        target: page.collection
                        function onUnsavedPresetUpdated() {
                            colorComboBox.updateColor()
                        }
                    }
                    onColorChanged: {
                        if (!modelData || modelData.colorChange)
                            return
                        page.collection.setValue(modelData.name, color)
                    }
                }
            }
        }
    }

    component PropertyItemDelegate: ItemDelegate {
        id: control
        ThemedItem.flat: true
        padding: 4
        leftPadding: 8
        rightPadding: 8
        background: Rectangle {
            implicitWidth: 100
            implicitHeight: 24
            property color _baseColor: control.highlighted ? Theme.accentColor : Theme.buttonColor
            color: !control.enabled ? control.ThemedItem.flat ? "transparent" : Theme.controlDisabledColorChange.apply(_baseColor) :
                    control.down && control.enabled ? Theme.controlPressedColorChange.apply(_baseColor) :
                        control.hovered && control.enabled ? Theme.controlHoveredColorChange.apply(_baseColor) :
                        control.highlighted ? Theme.accentColor : control.ThemedItem.flat ? "transparent" : Theme.buttonColor
            Behavior on color {
                ColorAnimation {
                    duration: Theme.colorAnimationDuration
                    easing.type: Easing.OutCubic
                }
            }
            border.width: control.visualFocus ? 2 : 0
            border.color: Theme.navigationColor
        }
    }

    ListModel {
        id: colorSchemePropertiesModel
        ListElement {
            name: "accentColor"
            text: qsTr("Accent Color")
        }
        ListElement {
            name: "warningColor"
            text: qsTr("Warning Color")
        }
        ListElement {
            name: "errorColor"
            text: qsTr("Error Color")
        }
        ListElement {
            name: "buttonColor"
            text: qsTr("Button Color")
        }
        ListElement {
            name: "textFieldColor"
            text: qsTr("Input Box Color")
        }
        ListElement {
            name: "scrollBarColor"
            text: qsTr("Scroll Bar Color")
        }
        ListElement {
            name: "borderColor"
            text: qsTr("Border Color")
        }
        ListElement {
            name: "backgroundPrimaryColor"
            text: qsTr("Primary Background Color")
        }
        ListElement {
            name: "backgroundSecondaryColor"
            text: qsTr("Secondary Background Color")
        }
        ListElement {
            name: "backgroundTertiaryColor"
            text: qsTr("Tertiary Background Color")
        }
        ListElement {
            name: "backgroundQuaternaryColor"
            text: qsTr("Quaternary Background Color")
        }
        ListElement {
            name: "splitterColor"
            text: qsTr("Splitter Color")
        }
        ListElement {
            name: "foregroundPrimaryColor"
            text: qsTr("Primary Foreground Color")
        }
        ListElement {
            name: "foregroundSecondaryColor"
            text: qsTr("Secondary Foreground Color")
        }
        ListElement {
            name: "linkColor"
            text: qsTr("Link Color")
        }
        ListElement {
            name: "navigationColor"
            text: qsTr("Navigation Color")
        }
        ListElement {
            name: "shadowColor"
            text: qsTr("Shadow Color")
        }
        ListElement {
            name: "highlightColor"
            text: qsTr("Highlight Color")
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 8
        spacing: 8
        RowLayout {
            Layout.fillWidth: true
            Label {
                text: qsTr("Preset")
            }
            ComboBox {
                Layout.fillWidth: true
            }
        }
        Rectangle {
            implicitHeight: 1
            Layout.fillWidth: true
            color: Theme.borderColor
        }
        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Frame {
                Layout.fillHeight: true
                Layout.fillWidth: true
                Layout.horizontalStretchFactor: 1
                ListView {
                    id: listView
                    anchors.fill: parent
                    anchors.margins: 1
                    focus: true
                    clip: true
                    model: colorSchemePropertiesModel
                    delegate: PropertyItemDelegate {
                        text: model.text
                        highlighted: ListView.isCurrentItem
                        width: ListView.view.width
                        onClicked: ListView.view.currentIndex = index
                    }
                }
            }
            StackLayout {
                Layout.fillHeight: true
                Layout.fillWidth: true
                Layout.horizontalStretchFactor: 1
                currentIndex: colorSchemePropertiesModel.get(listView.currentIndex)?.colorChange ? 1 : 0
                ScrollView {
                    id: colorScrollView
                    Layout.fillHeight: true
                    Layout.fillWidth: true
                    ColorPropertyEditor {
                        width: colorScrollView.width - 8
                    }
                }
                ScrollView {
                    id: colorChangeScrollView
                    Layout.fillHeight: true
                    Layout.fillWidth: true
                }
            }
        }
    }
}