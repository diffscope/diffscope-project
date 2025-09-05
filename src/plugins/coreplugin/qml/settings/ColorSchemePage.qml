import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQml.Models
import QtQuick.Dialogs

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
            if (page.started)
                page.pageHandle.markDirty()
        }
        function onAllPresetsChanged() {
            if (page.started)
                page.pageHandle.markDirty()
        }
        function onCurrentIndexChanged() {
            if (page.started)
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

    component ColorChangePropertyEditor: Pane {
        id: editor
        readonly property var modelData: colorSchemePropertiesModel.get(listView.currentIndex)
        property var colorChange: Theme.controlDisabledColorChange
        function updateColorChange() {
            if (!modelData || !modelData.colorChange)
                return
            colorChange = page.collection.value(modelData.name)
            editorStackLayout.currentIndex = 0
        }
        onModelDataChanged: updateColorChange();
        Connections {
            target: page.collection
            function onUnsavedPresetUpdated() {
                editor.updateColorChange()
            }
        }
        ColumnLayout {
            spacing: 8
            anchors.fill: parent
            GroupBox {
                title: qsTr("Properties")
                Layout.fillWidth: true
                Layout.fillHeight: true
                StackLayout {
                    id: editorStackLayout
                    anchors.fill: parent
                    ColumnLayout {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        Label {
                            Layout.fillWidth: true
                            text: page.pageHandle.colorChangeProperties(editor.colorChange)
                            visible: text !== ""
                            wrapMode: Text.Wrap
                        }
                        ToolButton {
                            Layout.fillWidth: true
                            flat: false
                            text: qsTr("Edit")
                            onClicked: editorStackLayout.currentIndex = 1
                        }
                    }
                    ColumnLayout {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        TextArea {
                            id: editorTextArea
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            text: page.pageHandle.colorChangePropertiesEditText(editor.colorChange)
                        }
                        ToolButton {
                            Layout.fillWidth: true
                            flat: false
                            text: qsTr("OK")
                            onClicked: () => {
                                if (!editor.modelData || !editor.colorChange)
                                    return
                                let colorChange;
                                try {
                                    colorChange = page.pageHandle.propertiesEditTextToColorChange(editorTextArea.text)
                                } catch (e) {
                                    MessageBox.critical("Syntax error", e.message)
                                    return
                                }
                                page.collection.setValue(modelData.name, colorChange)
                                editorStackLayout.currentIndex = 0
                            }
                        }
                    }
                }
            }
            GroupBox {
                title: qsTr("Preview")
                Layout.fillWidth: true
                ColumnLayout {
                    anchors.fill: parent
                    RowLayout {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        Label {
                            text: qsTr("Color")
                        }
                        ColorComboBox {
                            id: previewColorComboBox
                            Layout.fillWidth: true
                        }
                    }
                    ColorPreview {
                        implicitHeight: 24
                        Layout.fillWidth: true
                        color: editor.colorChange.apply(previewColorComboBox.color)
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

    component PresetComboBox: ComboBox {
        id: control

        delegate: ItemDelegate {
            id: itemDelegate
            required property var model
            required property int index

            width: ListView.view.width
            text: model[control.textRole]
            highlighted: control.currentIndex === index
            hoverEnabled: control.hoverEnabled

            contentItem: RowLayout {
                spacing: 16
                Text {
                    text: itemDelegate.text
                    font: itemDelegate.font
                    color: itemDelegate.down ? Theme.foregroundPressedColorChange.apply(Theme.foregroundPrimaryColor) :
                           itemDelegate.hovered ? Theme.foregroundHoveredColorChange.apply(Theme.foregroundPrimaryColor) :
                           Theme.foregroundPrimaryColor
                    Behavior on color {
                        ColorAnimation {
                            duration: Theme.colorAnimationDuration
                            easing.type: Easing.OutCubic
                        }
                    }
                }
                Text {
                    text: qsTr("Built-in")
                    visible: itemDelegate.model.data.internal && !itemDelegate.model.data.unsaved
                    font: itemDelegate.font
                    color: itemDelegate.down ? Theme.foregroundPressedColorChange.apply(Theme.foregroundSecondaryColor) :
                           itemDelegate.hovered ? Theme.foregroundHoveredColorChange.apply(Theme.foregroundSecondaryColor) :
                           Theme.foregroundSecondaryColor
                    Behavior on color {
                        ColorAnimation {
                            duration: Theme.colorAnimationDuration
                            easing.type: Easing.OutCubic
                        }
                    }
                }
                Item { Layout.fillWidth: true }
            }
        }
    }

    Popup {
        id: savePresetPopup
        anchors.centerIn: parent
        padding: 16
        width: 400
        property string originName: ""
        onAboutToShow: () => {
            presetNameTextField.text = originName
            presetNameTextField.forceActiveFocus()
        }
        ColumnLayout {
            anchors.fill: parent
            spacing: 8
            Label {
                text: qsTr("Preset name")
                Layout.fillWidth: true
                wrapMode: Text.Wrap
            }
            TextField {
                id: presetNameTextField
                Layout.fillWidth: true
                Keys.onReturnPressed: savePresetOkButton.animateClick()
            }
            Label {
                text: qsTr("Presets with the same name will be overwritten.")
                color: Theme.warningColor
                Layout.fillWidth: true
                opacity: savePresetPopup.visible && presetNameTextField.text !== savePresetPopup.originName && page.collection.presetExists(presetNameTextField.text) ? 1 : 0
                wrapMode: Text.Wrap
            }
            Button {
                id: savePresetOkButton
                ThemedItem.controlType: SVS.CT_Accent
                text: qsTr("OK")
                enabled: presetNameTextField.text !== "" && presetNameTextField.text !== savePresetPopup.originName
                Layout.fillWidth: true
                onClicked: () => {
                    if (savePresetPopup.originName === "") {
                        page.collection.savePreset(presetNameTextField.text)
                    } else {
                        page.collection.renamePreset(page.collection.currentIndex, presetNameTextField.text)
                    }
                    savePresetPopup.close()
                }
            }
        }
    }

    FileDialog {
        id: importFileDialog
        fileMode: FileDialog.OpenFile
        nameFilters: [qsTr("Color scheme files (*.dat)"), qsTr("All files (*)")]
        onAccepted: () => {
            page.collection.importPreset(page.Window.window, selectedFile)
        }
    }

    FileDialog {
        id: exportFileDialog
        fileMode: FileDialog.SaveFile
        nameFilters: [qsTr("Color scheme files (*.dat)"), qsTr("All files (*)")]
        onAccepted: () => {
            page.collection.exportPreset(page.Window.window, selectedFile)
        }
    }

    ListModel {
        id: colorSchemePropertiesModel
        ListElement {
            name: "accentColor"
            text: qsTr("Accent color")
        }
        ListElement {
            name: "warningColor"
            text: qsTr("Warning color")
        }
        ListElement {
            name: "errorColor"
            text: qsTr("Error color")
        }
        ListElement {
            name: "buttonColor"
            text: qsTr("Button color")
        }
        ListElement {
            name: "textFieldColor"
            text: qsTr("Input box color")
        }
        ListElement {
            name: "scrollBarColor"
            text: qsTr("Scroll bar color")
        }
        ListElement {
            name: "borderColor"
            text: qsTr("Border color")
        }
        ListElement {
            name: "backgroundPrimaryColor"
            text: qsTr("Primary background color")
        }
        ListElement {
            name: "backgroundSecondaryColor"
            text: qsTr("Secondary background color")
        }
        ListElement {
            name: "backgroundTertiaryColor"
            text: qsTr("Tertiary background color")
        }
        ListElement {
            name: "backgroundQuaternaryColor"
            text: qsTr("Quaternary background color")
        }
        ListElement {
            name: "splitterColor"
            text: qsTr("Splitter color")
        }
        ListElement {
            name: "foregroundPrimaryColor"
            text: qsTr("Primary foreground color")
        }
        ListElement {
            name: "foregroundSecondaryColor"
            text: qsTr("Secondary foreground color")
        }
        ListElement {
            name: "linkColor"
            text: qsTr("Link color")
        }
        ListElement {
            name: "navigationColor"
            text: qsTr("Navigation color")
        }
        ListElement {
            name: "shadowColor"
            text: qsTr("Shadow color")
        }
        ListElement {
            name: "highlightColor"
            text: qsTr("Highlight color")
        }
        ListElement {
            name: "flatButtonHighContrastBorderColor"
            text: qsTr("Flat button high contrast border color")
        }
        ListElement {
            name: "controlDisabledColorChange"
            text: qsTr("Control disabled color change")
            colorChange: true
        }
        ListElement {
            name: "foregroundDisabledColorChange"
            text: qsTr("Foreground disabled color change")
            colorChange: true
        }
        ListElement {
            name: "controlHoveredColorChange"
            text: qsTr("Control hovered color change")
            colorChange: true
        }
        ListElement {
            name: "foregroundHoveredColorChange"
            text: qsTr("Foreground hovered color change")
            colorChange: true
        }
        ListElement {
            name: "controlPressedColorChange"
            text: qsTr("Control pressed color change")
            colorChange: true
        }
        ListElement {
            name: "foregroundPressedColorChange"
            text: qsTr("Foreground pressed color change")
            colorChange: true
        }
        ListElement {
            name: "controlCheckedColorChange"
            text: qsTr("Control checked color change")
            colorChange: true
        }
        ListElement {
            name: "annotationPopupTitleColorChange"
            text: qsTr("Annotation popup title color change")
            colorChange: true
        }
        ListElement {
            name: "annotationPopupContentColorChange"
            text: qsTr("Annotation popup content color change")
            colorChange: true
        }
        ListElement {
            name: "dockingPanelHeaderActiveColorChange"
            text: qsTr("Docking panel header active color change")
            colorChange: true
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
            PresetComboBox {
                id: presetComboBox
                Layout.fillWidth: true
                model: page.collection.allPresets
                textRole: "name"
                valueRole: "data"
                currentIndex: page.collection.currentIndex
                onActivated: (index) => {
                    if (index === page.collection.currentIndex)
                        return
                    page.collection.loadPreset(index)
                }
            }
            Button {
                flat: true
                icon.source: "qrc:/diffscope/coreplugin/icons/MoreHorizontal16Filled.svg"
                display: AbstractButton.IconOnly
                text: qsTr("Preset Actions")
                action: MenuAction {
                    menu: Menu {
                        Action {
                            text: qsTr("Save As...")
                            onTriggered: () => {
                                savePresetPopup.originName = ""
                                savePresetPopup.open()
                            }
                        }
                        Action {
                            enabled: !presetComboBox.currentValue.internal
                            text: qsTr("Rename...")
                            onTriggered: () => {
                                savePresetPopup.originName = presetComboBox.currentText
                                savePresetPopup.open()
                            }
                        }
                        Action {
                            enabled: !presetComboBox.currentValue.internal
                            text: qsTr("Delete")
                            onTriggered: () => {
                                page.collection.removePreset(presetComboBox.currentIndex)
                            }
                        }
                        MenuSeparator {}
                        Action {
                            text: qsTr("Import from File...")
                            onTriggered: () => {
                                importFileDialog.open()
                            }
                        }
                        Action {
                            enabled: !presetComboBox.currentValue.unsaved
                            text: qsTr("Export to File...")
                            onTriggered: () => {
                                exportFileDialog.open()
                            }
                        }
                    }
                }
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
                ColorPropertyEditor {
                    Layout.fillHeight: true
                    Layout.fillWidth: true
                }
                ColorChangePropertyEditor {
                    Layout.fillHeight: true
                    Layout.fillWidth: true
                }
            }
        }
    }
}