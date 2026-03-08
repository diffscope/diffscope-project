import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQml.Models
import QtQuick.Dialogs

import SVSCraft
import SVSCraft.UIComponents
import SVSCraft.UIComponents.impl

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
        ThemedItem.controlType: highlighted ? SVS.CT_Accent : SVS.CT_Normal
        background: ButtonRectangle {
            implicitWidth: 100
            implicitHeight: 24
            control: control
            checked: control.highlighted
            flat: true
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
                        page.collection.renamePreset(page.collection.realCurrentIndex, presetNameTextField.text)
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
            name: "paneSeparatorColor"
            text: qsTr("Pane separator color")
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
        ListElement {
            name: "trackColorSchema0"
            text: qsTr("Track color 1")
        }
        ListElement {
            name: "trackColorSchema1"
            text: qsTr("Track color 2")
        }
        ListElement {
            name: "trackColorSchema2"
            text: qsTr("Track color 3")
        }
        ListElement {
            name: "trackColorSchema3"
            text: qsTr("Track color 4")
        }
        ListElement {
            name: "trackColorSchema4"
            text: qsTr("Track color 5")
        }
        ListElement {
            name: "trackColorSchema5"
            text: qsTr("Track color 6")
        }
        ListElement {
            name: "trackColorSchema6"
            text: qsTr("Track color 7")
        }
        ListElement {
            name: "trackColorSchema7"
            text: qsTr("Track color 8")
        }
        ListElement {
            name: "trackColorSchema8"
            text: qsTr("Track color 9")
        }
        ListElement {
            name: "trackColorSchema9"
            text: qsTr("Track color 10")
        }
        ListElement {
            name: "trackColorSchema10"
            text: qsTr("Track color 11")
        }
        ListElement {
            name: "trackColorSchema11"
            text: qsTr("Track color 12")
        }
        ListElement {
            name: "loopColor"
            text: qsTr("Loop range slider color")
        }
        ListElement {
            name: "levelMeterColor"
            text: qsTr("Level meter background color")
        }
        ListElement {
            name: "editAreaPrimaryColor"
            text: qsTr("Edit area primary background color")
        }
        ListElement {
            name: "editAreaSecondaryColor"
            text: qsTr("Edit area secondary background color")
        }
        ListElement {
            name: "editAreaPrimaryHighlightColor"
            text: qsTr("Edit area primary highlight background color")
        }
        ListElement {
            name: "editAreaSecondaryHighlightColor"
            text: qsTr("Edit area secondary highlight background color")
        }
        ListElement {
            name: "playheadPrimaryColor"
            text: qsTr("Playhead primary color")
        }
        ListElement {
            name: "playheadSecondaryColor"
            text: qsTr("Playhead secondary color")
        }
        ListElement {
            name: "cursorIndicatorColor"
            text: qsTr("Cursor indicator color")
        }
        ListElement {
            name: "scissorIndicatorColor"
            text: qsTr("Scissor indicator color")
        }
        ListElement {
            name: "scalePrimaryColor"
            text: qsTr("Scale primary color")
        }
        ListElement {
            name: "scaleSecondaryColor"
            text: qsTr("Scale secondary color")
        }
        ListElement {
            name: "scaleTertiaryColor"
            text: qsTr("Scale tertiary color")
        }
        ListElement {
            name: "levelLowColor"
            text: qsTr("Level meter low level color")
        }
        ListElement {
            name: "levelMediumColor"
            text: qsTr("Level meter medium level color")
        }
        ListElement {
            name: "levelHighColor"
            text: qsTr("Level meter high level color")
        }
        ListElement {
            name: "muteColor"
            text: qsTr("Mute button color")
        }
        ListElement {
            name: "soloColor"
            text: qsTr("Solo button color")
        }
        ListElement {
            name: "recordColor"
            text: qsTr("Record button color")
        }
        ListElement {
            name: "routeColor"
            text: qsTr("Multi-channel output button color")
        }
        ListElement {
            name: "clipMuteColor"
            text: qsTr("Mute clip color")
        }
        ListElement {
            name: "whiteKeyColor"
            text: qsTr("Piano white key color")
        }
        ListElement {
            name: "blackKeyColor"
            text: qsTr("Piano black key color")
        }
        ListElement {
            name: "whiteKeyTextColor"
            text: qsTr("Text on piano white key color")
        }
        ListElement {
            name: "blackKeyTextColor"
            text: qsTr("Text on piano black key color")
        }
        ListElement {
            name: "itemSelectedColorChange"
            text: qsTr("Item selected color change")
            colorChange: true
        }
        ListElement {
            name: "clipSelectedColorChange"
            text: qsTr("Clip selected color change")
            colorChange: true
        }
        ListElement {
            name: "clipThumbnailColorChange"
            text: qsTr("Clip thumbnail color change")
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
                currentIndex: page.collection.visualCurrentIndex
                readonly property int realCurrentIndex: currentIndex + (currentValue?.indexOffset ?? 0)
                onActivated: (index) => {
                    if (index === page.collection.visualCurrentIndex)
                        return
                    page.collection.loadPreset(index + valueAt(index).indexOffset)
                }
            }
            Button {
                flat: true
                icon.source: "image://fluent-system-icons/more_horizontal"
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
                                page.collection.removePreset(presetComboBox.realCurrentIndex)
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