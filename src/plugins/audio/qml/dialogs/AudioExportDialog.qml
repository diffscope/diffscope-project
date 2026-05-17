import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Controls.impl

import SVSCraft
import SVSCraft.UIComponents

Window {
    id: dialog

    required property ExportAudioAddOn addOn
    required property QtObject exporter
    required property int timeRangeAllEnd
    required property int timeRangeLoopSectionStart
    required property int timeRangeLoopSectionEnd

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

    function setTrackChecked(trackIndex, checked) {
        let source = Array.from(dialog.addOn.currentParameter.source)
        const index = source.indexOf(trackIndex)
        if (checked && index < 0)
            source.push(trackIndex)
        else if (!checked && index >= 0)
            source.splice(index, 1)
        source.sort((a, b) => a - b)
        dialog.addOn.currentParameter.source = source
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

    Component.onCompleted: sampleRateCombo.editText = AudioExporterPresets.currentConfig.formatSampleRate.toLocaleString()

    component AssistantButton: IconImage {
        id: assistantButton

        source: "image://fluent-system-icons/question_circle?style=regular"
        sourceSize.width: 16
        sourceSize.height: 16
        color: Theme.foregroundSecondaryColor

        property string text: ""

        ToolTip {
            id: assistantToolTip
            text: assistantButton.text
            visible: hoverHandler.hovered
        }

        HoverHandler {
            id: hoverHandler
        }
    }

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
        onAboutToShow: () => {
            presetNameTextField.text = dialog.currentPresetBuiltin() || dialog.currentPresetUnsaved() ? "" : dialog.currentPresetName()
            presetNameTextField.forceActiveFocus()
            presetNameTextField.selectAll()
        }
    }

    Dialog {
        id: dryRunDialog
        anchors.centerIn: parent
        title: qsTr("Dry Run")
        width: 400
        property string fileTypeName: ""
        property string durationText: ""
        property string sizeText: ""
        property list<string> warningTextList: []
        property list<string> fileList: []
        ColumnLayout {
            anchors.fill: parent
            Label {
                Layout.fillWidth: true
                wrapMode: Text.Wrap
                text: qsTr("%Ln %1 file(s)\nDuration: %2\nEstimated size of each file: %3", "", dryRunDialog.fileList.length).arg(dryRunDialog.fileTypeName).arg(dryRunDialog.durationText).arg(dryRunDialog.sizeText)
            }
            ColumnLayout {
                Layout.fillWidth: true
                Repeater {
                    model: dryRunDialog.warningTextList
                    Annotation {
                        required property string modelData
                        ThemedItem.controlType: SVS.CT_Warning
                        Layout.fillWidth: true
                        label: modelData
                    }
                }
            }
            GroupBox {
                Layout.fillWidth: true
                title: qsTr("File List")
                Label {
                    anchors.fill: parent
                    text: dryRunDialog.fileList.map(v => AudioQmlHelper.getNativeSeparatorPath(v)).join("\n")
                }
            }
        }
        onAboutToShow: () => {
            fileList = dialog.exporter.fileList
            warningTextList = dialog.exporter.preflightWarningTexts(dialog.exporter.preflightWarnings)
            fileTypeName = [qsTr("WAV"), qsTr("FLAC"), qsTr("Ogg Vorbis"), qsTr("MP3")][AudioExporterPresets.currentConfig.fileType]
            const msec = dialog.addOn.calculateDurationInMsec(dialog.exporter)
            const minutes = Math.floor(msec / 60000);
            const seconds = Math.floor((msec % 60000) / 1000);
            const milliseconds = Math.floor(msec % 1000);
            durationText = `${minutes}:${String(seconds).padStart(2, '0')}.${String(milliseconds).padStart(3, '0')}`;
            if (AudioExporterPresets.currentConfig.fileType !== 0) {
                sizeText = qsTr("N/A")
            } else {
                const estimatedBytes = msec / 1000 * AudioExporterPresets.currentConfig.formatSampleRate * (AudioExporterPresets.currentConfig.formatMono ? 1 : 2) * (4 - AudioExporterPresets.currentConfig.formatOption)
                const kib = estimatedBytes / 1024;
                const mib = kib / 1024;
                if (estimatedBytes < 1024) {
                    sizeText = `${estimatedBytes} B`;
                } else if (kib < 1024) {
                    sizeText = `${kib.toFixed(2).replace(/\.?0+$/, "")} KiB`;
                } else {
                    sizeText = `${mib.toFixed(2).replace(/\.?0+$/, "")} MiB`;
                }
            }
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
                                        action: MenuAction {
                                            menu: Menu {
                                                Action {
                                                    text: qsTr("Project name")
                                                    onTriggered: dialog.addOn.appendFileNameTemplate("${projectName}")
                                                }
                                                Action {
                                                    text: qsTr("Track name")
                                                    enabled: AudioExporterPresets.currentConfig.mixingOption !== 0
                                                    onTriggered: dialog.addOn.appendFileNameTemplate("${trackName}")
                                                }
                                                Action {
                                                    text: qsTr("Track number")
                                                    enabled: AudioExporterPresets.currentConfig.mixingOption !== 0
                                                    onTriggered: dialog.addOn.appendFileNameTemplate("${trackIndex}")
                                                }
                                            }
                                        }
                                    }
                                    AssistantButton {
                                        id: fileNameAssistant
                                        text: "<h3>Template tags</h3><br/><p><b>${projectName}</b>: the base name of the project file</p><br/><p>The following tags are available only for separate-track export:</p><br/><p><b>${trackName}</b>: track name<br/><b>${trackIndex}</b>: track number</p>"
                                    }
                                }
                                columnItem: TextField {
                                    text: AudioExporterPresets.currentConfig.fileName
                                    onTextEdited: AudioExporterPresets.currentConfig.fileName = text
                                    Accessible.description: fileNameAssistant.text
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
                            text: AudioQmlHelper.getNativeSeparatorPath(dialog.exporter.fileList[0] ?? "")
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
                                    onActivated: (index) => dialog.addOn.setFileType(index)
                                }
                                Layout.fillWidth: true
                            }
                            FormGroup {
                                id: optionFormGroup
                                label: qsTr("Option")
                                enabled: dialog.addOn.formatOptions(AudioExporterPresets.currentConfig.fileType).length > 0
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
                                id: qualityFormGroup
                                label: qsTr("Quality")
                                enabled: AudioExporterPresets.currentConfig.fileType !== 0 && AudioExporterPresets.currentConfig.fileType !== 1
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
                                        qsTr("All tracks"),
                                        qsTr("Selected tracks"),
                                        qsTr("Custom"),
                                    ]
                                    currentIndex: AudioExporterPresets.currentConfig.sourceOption
                                    onActivated: (index) => AudioExporterPresets.currentConfig.sourceOption = index
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
                                        required property var modelData
                                        required property int index
                                        topPadding: 0
                                        bottomPadding: 0
                                        text: qsTr("%L1: %2").arg(index + 1).arg(modelData.name)
                                        checked: {
                                            let sourceOption = AudioExporterPresets.currentConfig.sourceOption
                                            if (sourceOption === 0) {
                                                return true
                                            } else if (sourceOption === 1) {
                                                return dialog.trackSelectionModel.isItemSelected(modelData)
                                            } else {
                                                return dialog.addOn.currentParameter.source.includes(index)
                                            }
                                        }
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
                            text: qsTr("Whole project")
                            checked: AudioExporterPresets.currentConfig.timeRange === 0
                            onClicked: AudioExporterPresets.currentConfig.timeRange = 0
                        }
                        RadioButton {
                            text: qsTr("Loop section")
                            checked: AudioExporterPresets.currentConfig.timeRange === 1
                            onClicked: AudioExporterPresets.currentConfig.timeRange = 1
                        }
                        RadioButton {
                            text: qsTr("Custom")
                            checked: AudioExporterPresets.currentConfig.timeRange === 2
                            onClicked: AudioExporterPresets.currentConfig.timeRange = 2
                        }
                        MusicTimeSpinBox {
                            id: startSpinBox
                            Layout.fillWidth: true
                            timeline: dialog.addOn.windowHandle.projectTimeline.musicTimeline
                            enabled: AudioExporterPresets.currentConfig.timeRange === 2
                            value: {
                                if (AudioExporterPresets.currentConfig.timeRange === 0) {
                                    return 0
                                } else if (AudioExporterPresets.currentConfig.timeRange === 1) {
                                    return dialog.timeRangeLoopSectionStart
                                }
                                return dialog.addOn.currentParameter.rangeStart
                            }
                            onValueModified: dialog.addOn.currentParameter.rangeStart = value
                        }
                        Label {
                            text: "-"
                        }
                        MusicTimeSpinBox {
                            id: endSpinBox
                            Layout.fillWidth: true
                            timeline: dialog.addOn.windowHandle.projectTimeline.musicTimeline
                            enabled: AudioExporterPresets.currentConfig.timeRange === 2
                            from: dialog.addOn.currentParameter.rangeStart + 1
                            value: {
                                if (AudioExporterPresets.currentConfig.timeRange === 0) {
                                    return dialog.timeRangeAllEnd
                                } else if (AudioExporterPresets.currentConfig.timeRange === 1) {
                                    return dialog.timeRangeLoopSectionEnd
                                }
                                return dialog.addOn.currentParameter.rangeStart + dialog.addOn.currentParameter.rangeLength
                            }
                            onValueModified: dialog.addOn.currentParameter.rangeLength = value - dialog.addOn.currentParameter.rangeStart
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
                    onClicked: dryRunDialog.open()
                }
                CheckBox {
                    id: keepOpenCheckBox
                    text: qsTr("Keep this dialog open after successful export")
                }
                Item {
                    Layout.fillWidth: true
                }
                Button {
                    id: warningButton
                    flat: true
                    icon.source: "image://fluent-system-icons/warning"
                    icon.color: Theme.warningColor
                    text: qsTr("Warnings")
                    display: AbstractButton.IconOnly
                    visible: dialog.exporter.preflightWarnings !== 0
                    onClicked: dryRunDialog.open()
                }
                Button {
                    id: exportButton
                    text: qsTr("Export")
                    ThemedItem.controlType: SVS.CT_Accent
                    onClicked: () => {
                        for (const warningText of dialog.exporter.preflightWarningTexts(dialog.exporter.preflightWarnings)) {
                            if (MessageBox.warning(qsTr("Warning"), warningText + "\n\n" + qsTr("Continue to export?"), SVS.Yes | SVS.No, SVS.No) === SVS.No) {
                                return
                            }
                        }
                        if (dialog.addOn.runExport(dialog.exporter) && !keepOpenCheckBox.checked) {
                            dialog.close()
                        }
                    }
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
