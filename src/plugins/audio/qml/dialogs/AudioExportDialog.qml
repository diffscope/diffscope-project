import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SVSCraft
import SVSCraft.UIComponents

Window {
    id: dialog

    required property ExportAudioAddOn addOn
    readonly property QtObject trackList: addOn.windowHandle.projectDocumentContext.document.model.tracks
    readonly property var tracks: trackList.items
    readonly property QtObject trackSelectionModel: addOn.windowHandle.projectDocumentContext.document.selectionModel.trackSelectionModel
    property int trackSelectionRevision: 0

    width: 800
    height: windowLayout.implicitHeight
    minimumHeight: windowLayout.implicitHeight
    flags: Qt.Dialog | Qt.CustomizeWindowHint | Qt.WindowTitleHint | Qt.WindowSystemMenuHint | Qt.WindowCloseButtonHint
    modality: Qt.WindowModal
    title: qsTr("Export Audio")

    signal finished()
    onClosing: finished()

    function numberFromText(text) {
        return Number.fromLocaleString(Qt.locale(), text)
    }

    function selectedTrackIndexesFromProject() {
        return tracks.map((track, index) => trackSelectionModel.isItemSelected(track) ? index : -1).filter(index => index >= 0)
    }

    function effectiveSourceIndexes() {
        if (AudioExporterPresets.currentConfig.sourceOption === 0)
            return tracks.map((track, index) => index)
        if (AudioExporterPresets.currentConfig.sourceOption === 1)
            return selectedTrackIndexesFromProject()
        return AudioExporterPresets.currentConfig.source
    }

    function isTrackChecked(trackIndex) {
        trackSelectionRevision
        return effectiveSourceIndexes().indexOf(trackIndex) >= 0
    }

    function setSourceOption(sourceOption) {
        const source = sourceOption === 2 ? effectiveSourceIndexes() :
                                           (sourceOption === 0 ? tracks.map((track, index) => index) : selectedTrackIndexesFromProject())
        AudioExporterPresets.currentConfig.sourceOption = sourceOption
        AudioExporterPresets.currentConfig.source = source
    }

    function setTrackChecked(trackIndex, checked) {
        let source = Array.from(AudioExporterPresets.currentConfig.source)
        const index = source.indexOf(trackIndex)
        if (checked && index < 0)
            source.push(trackIndex)
        else if (!checked && index >= 0)
            source.splice(index, 1)
        source.sort((a, b) => a - b)
        AudioExporterPresets.currentConfig.source = source
    }

    function presetData(role) {
        return AudioExporterPresets.model.data(AudioExporterPresets.model.index(AudioExporterPresets.currentIndex, 0), role)
    }

    function currentPresetName() {
        return presetData(AudioExporterPresets.NameRole)
    }

    function currentPresetBuiltin() {
        return presetData(AudioExporterPresets.BuiltinRole)
    }

    function currentPresetUnsaved() {
        return presetData(AudioExporterPresets.UnsavedRole)
    }

    Connections {
        target: AudioExporterPresets
        function onCurrentConfigChanged() {
            sampleRateCombo.editText = AudioExporterPresets.currentConfig.formatSampleRate.toLocaleString()
        }
    }

    Connections {
        target: dialog.trackSelectionModel
        function onSelectedItemsChanged() {
            ++dialog.trackSelectionRevision
            if (AudioExporterPresets.currentConfig.sourceOption === 1)
                dialog.setSourceOption(1)
        }
    }

    Connections {
        target: dialog.trackList
        function onItemsChanged() {
            if (AudioExporterPresets.currentConfig.sourceOption !== 2)
                dialog.setSourceOption(AudioExporterPresets.currentConfig.sourceOption)
        }
    }

    Component.onCompleted: sampleRateCombo.editText = AudioExporterPresets.currentConfig.formatSampleRate.toLocaleString()

    Dialog {
        id: saveAsDialog
        anchors.centerIn: parent
        title: qsTr("Save Preset As")
        property bool overwrite: false
        width: 400
        ColumnLayout {
            anchors.fill: parent
            Label {
                Layout.fillWidth: true
                wrapMode: Text.Wrap
                text: qsTr("Preset name")
            }
            TextField {
                id: presetNameTextField
                Layout.fillWidth: true
                ThemedItem.controlType: saveAsDialog.overwrite ? SVS.CT_Warning : SVS.CT_Normal
                onTextChanged: saveAsDialog.overwrite = AudioExporterPresets.hasCustomPreset(text)
            }
            Label {
                Layout.fillWidth: true
                wrapMode: Text.Wrap
                text: qsTr("The preset with the same name will be overwritten.")
                visible: saveAsDialog.overwrite
            }
            RowLayout {
                Layout.fillWidth: true
                Item {
                    Layout.fillWidth: true
                }
                Button {
                    text: qsTr("OK")
                    enabled: presetNameTextField.text.trim().length > 0
                    onClicked: {
                        AudioExporterPresets.saveCurrentAsCustomPreset(presetNameTextField.text)
                        saveAsDialog.close()
                    }
                }
            }
        }
        onOpened: {
            presetNameTextField.text = dialog.currentPresetBuiltin() || dialog.currentPresetUnsaved() ? "" : dialog.currentPresetName()
            presetNameTextField.forceActiveFocus()
            presetNameTextField.selectAll()
        }
    }

    ColumnLayout {
        id: windowLayout
        anchors.fill: parent
        spacing: 0
        Rectangle {
            color: Theme.backgroundPrimaryColor
            Layout.fillWidth: true
            Layout.fillHeight: true
            implicitHeight: mainLayout.implicitHeight + 24
            ColumnLayout {
                id: mainLayout
                anchors.fill: parent
                anchors.margins: 12
                spacing: 8
                GroupBox {
                    id: presetGrouopBox
                    title: qsTr("Preset")
                    Layout.fillWidth: true
                    RowLayout {
                        anchors.fill: parent
                        ComboBox {
                            id: presetComboBox
                            Layout.fillWidth: true
                            model: AudioExporterPresets.model
                            textRole: "name"
                            currentIndex: AudioExporterPresets.currentIndex
                            onActivated: (index) => AudioExporterPresets.currentIndex = index
                        }
                        Button {
                            text: qsTr("Save &As...")
                            onClicked: saveAsDialog.open()
                        }
                        Button {
                            text: qsTr("&Delete")
                            enabled: !dialog.currentPresetBuiltin() && !dialog.currentPresetUnsaved()
                            onClicked: AudioExporterPresets.removeCustomPreset(dialog.currentPresetName())
                        }
                    }
                }
                GroupBox {
                    id: fileGroupBox
                    title: qsTr("File")
                    Layout.fillWidth: true
                    ColumnLayout {
                        anchors.fill: parent
                        Button {
                            text: qsTr("&Browse...")
                            icon.source: "image://fluent-system-icons/folder_open"
                            Layout.fillWidth: true
                            onClicked: dialog.addOn.browseFile()
                        }
                        RowLayout {
                            Layout.fillWidth: true
                            uniformCellSizes: true
                            FormGroup {
                                label: qsTr("Name")
                                rowItem: RowLayout {
                                    ToolButton {
                                        icon.source: "image://fluent-system-icons/braces"
                                        text: qsTr("Templates")
                                        display: AbstractButton.IconOnly
                                    }
                                }
                                columnItem: TextField {
                                    text: AudioExporterPresets.currentConfig.fileName
                                    onTextEdited: AudioExporterPresets.currentConfig.fileName = text
                                }
                                Layout.fillWidth: true
                            }
                            FormGroup {
                                label: qsTr("Directory")
                                columnItem: TextField {
                                    placeholderText: qsTr("(Project directory)")
                                    text: AudioExporterPresets.currentConfig.fileDirectory
                                    onTextEdited: AudioExporterPresets.currentConfig.fileDirectory = text
                                }
                                Layout.fillWidth: true
                            }
                        }
                        Label {
                            Layout.fillWidth: true
                            wrapMode: Text.Wrap
                        }
                    }
                }
                GroupBox {
                    id: formatGroupBox
                    title: qsTr("Format")
                    Layout.fillWidth: true
                    ColumnLayout {
                        anchors.fill: parent
                        RowLayout {
                            Layout.fillWidth: true
                            uniformCellSizes: true
                            FormGroup {
                                label: qsTr("Type")
                                columnItem: ComboBox {
                                    id: fileTypeComboBox
                                    model: [
                                        qsTr("WAV"),
                                        qsTr("FLAC"),
                                        qsTr("Ogg Vorbis"),
                                        qsTr("MP3"),
                                    ]
                                    currentIndex: AudioExporterPresets.currentConfig.fileType
                                    onActivated: (index) => {
                                        AudioExporterPresets.currentConfig.fileType = index
                                        AudioExporterPresets.currentConfig.formatOption = 0
                                    }
                                }
                                Layout.fillWidth: true
                            }
                            FormGroup {
                                label: qsTr("Option")
                                columnItem: ComboBox {
                                    model: dialog.addOn.formatOptions(AudioExporterPresets.currentConfig.fileType)
                                    currentIndex: AudioExporterPresets.currentConfig.formatOption
                                    onActivated: (index) => AudioExporterPresets.currentConfig.formatOption = index
                                }
                                Layout.fillWidth: true
                            }
                        }
                        RowLayout {
                            Layout.fillWidth: true
                            uniformCellSizes: true
                            FormGroup {
                                label: qsTr("Channel")
                                columnItem: ComboBox {
                                    model: [
                                        qsTr("Stereo"),
                                        qsTr("Mono"),
                                    ]
                                    currentIndex: AudioExporterPresets.currentConfig.formatMono ? 1 : 0
                                    onActivated: (index) => AudioExporterPresets.currentConfig.formatMono = index === 1
                                }
                                Layout.fillWidth: true
                            }
                            FormGroup {
                                label: qsTr("Sample rate")
                                columnItem: ComboBox {
                                    id: sampleRateCombo
                                    model: [8000, 11025, 12000, 16000, 22050, 24000, 32000, 44100, 48000, 64000, 88200, 96000, 128000, 176400, 192000, 256000, 352800, 384000].map(v => v.toLocaleString())
                                    editable: true
                                    validator: DoubleValidator {
                                        top: Number.MAX_VALUE
                                        bottom: 0.01
                                        decimals: 2
                                    }
                                    onActivated: AudioExporterPresets.currentConfig.formatSampleRate = dialog.numberFromText(currentText)
                                    onAccepted: {
                                        const value = dialog.numberFromText(editText)
                                        if (value > 0)
                                            AudioExporterPresets.currentConfig.formatSampleRate = value
                                    }
                                }
                                Layout.fillWidth: true
                            }
                            FormGroup {
                                label: qsTr("Quality")
                                columnItem: RowLayout {
                                    Slider {
                                        Layout.fillWidth: true
                                        from: 0
                                        to: 100
                                        stepSize: 1
                                        value: AudioExporterPresets.currentConfig.formatQuality
                                        onMoved: AudioExporterPresets.currentConfig.formatQuality = Math.round(value)
                                    }
                                    SpinBox {
                                        implicitWidth: 64
                                        from: 0
                                        to: 100
                                        value: AudioExporterPresets.currentConfig.formatQuality
                                        onValueModified: AudioExporterPresets.currentConfig.formatQuality = value
                                    }
                                }
                                Layout.fillWidth: true
                            }
                        }
                    }

                }
                GroupBox {
                    id: mixerGroupBox
                    title: qsTr("Mixer")
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    ColumnLayout {
                        anchors.fill: parent
                        RowLayout {
                            Layout.fillWidth: true
                            uniformCellSizes: true
                            FormGroup {
                                label: qsTr("Mixing option")
                                columnItem: ComboBox {
                                    model: [
                                        qsTr("Mixed"),
                                        qsTr("Separated"),
                                        qsTr("Separated (through master track)"),
                                    ]
                                    currentIndex: AudioExporterPresets.currentConfig.mixingOption
                                    onActivated: (index) => dialog.addOn.setMixingOption(index)
                                }
                                Layout.fillWidth: true
                            }
                            FormGroup {
                                label: qsTr("Source")
                                columnItem: ComboBox {
                                    model: [
                                        qsTr("All"),
                                        qsTr("Selected tracks"),
                                        qsTr("Custom"),
                                    ]
                                    currentIndex: AudioExporterPresets.currentConfig.sourceOption
                                    onActivated: (index) => dialog.setSourceOption(index)
                                }
                                Layout.fillWidth: true
                            }
                            FormGroup {
                                label: qsTr("Mute/Solo")
                                columnItem: ComboBox {
                                    model: [
                                        qsTr("Enable"),
                                        qsTr("Bypass")
                                    ]
                                    currentIndex: AudioExporterPresets.currentConfig.muteSoloEnabled ? 0 : 1
                                    onActivated: (index) => AudioExporterPresets.currentConfig.muteSoloEnabled = index === 0
                                }
                                Layout.fillWidth: true
                            }
                        }
                        Frame {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            ThemedItem.backgroundLevel: SVS.BL_Quaternary
                            enabled: AudioExporterPresets.currentConfig.sourceOption === 2
                            padding: 4
                            ColumnLayout {
                                anchors.left: parent.left
                                anchors.right: parent.right
                                spacing: 8
                                Repeater {
                                    model: dialog.tracks
                                    CheckBox {
                                        topPadding: 0
                                        bottomPadding: 0
                                        text: qsTr("%L1: %2").arg(index + 1).arg(modelData.name)
                                        checked: dialog.isTrackChecked(index)
                                        onClicked: dialog.setTrackChecked(index, checked)
                                    }
                                }
                            }
                        }
                    }
                }
                GroupBox {
                    id: timeRangeGroupBox
                    title: qsTr("Time Range")
                    Layout.fillWidth: true
                    RowLayout {
                        spacing: 12
                        RadioButton {
                            text: qsTr("All")
                            checked: AudioExporterPresets.currentConfig.timeRange === 0
                            onClicked: AudioExporterPresets.currentConfig.timeRange = 0
                        }
                        RadioButton {
                            text: qsTr("Loop section")
                            checked: AudioExporterPresets.currentConfig.timeRange === 1
                            onClicked: AudioExporterPresets.currentConfig.timeRange = 1
                        }
                    }
                }
            }
        }
        Rectangle {
            implicitHeight: 1
            Layout.fillWidth: true
            color: Theme.paneSeparatorColor
        }
        Rectangle {
            color: Theme.backgroundSecondaryColor
            Layout.fillWidth: true
            height: 52
            RowLayout {
                id: buttonLayout
                anchors.fill: parent
                anchors.margins: 12
                Button {
                    id: dryRunButton
                    text: qsTr("Dry Run")
                }
                Item {
                    Layout.fillWidth: true
                }
                Button {
                    id: exportButton
                    text: qsTr("Export")
                    ThemedItem.controlType: SVS.CT_Accent
                }
                Button {
                    id: cancelButton
                    text: qsTr("Cancel")
                    onClicked: dialog.close()
                }
            }
        }
    }
}
