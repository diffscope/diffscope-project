// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SVSCraft
import SVSCraft.UIComponents

import DiffScope.Synth

ColumnLayout {
    id: view

    required property TempoExtractionPreview preview
    readonly property bool compactSettings: width < 700
    readonly property real firstLabelWidth: Math.max(referenceLabel.implicitWidth, alignmentLabel.implicitWidth,
                                                    compactSettings ? positionLabel.implicitWidth : 0)
    readonly property real secondLabelWidth: positionLabel.implicitWidth
    spacing: 8

    function focusFirstControl() {
        modeBar.currentItem.forceActiveFocus()
    }

    Keys.onSpacePressed: event => {
        preview.togglePlayback()
        event.accepted = true
    }

    component Playhead: Rectangle {
        required property TempoExtractionPreview preview
        width: 1
        height: parent.height
        color: Theme.foregroundPrimaryColor
        visible: preview.playhead >= preview.viewStart && preview.playhead <= preview.viewEnd
        x: (preview.playhead - preview.viewStart) / (preview.viewEnd - preview.viewStart) * parent.width
    }

    component ValueAxis: Item {
        id: axis
        required property real minimum
        required property real maximum
        property real valueScale: 1
        property int decimals: 0
        readonly property real zeroPosition: maximum / (maximum - minimum) * height
        readonly property real middle: minimum < 0 && maximum > 0 && zeroPosition >= metrics.height + 4 && zeroPosition <= height - metrics.height - 4 ? 0 : (minimum + maximum) / 2

        function format(value) {
            return (value / valueScale).toLocaleString(Qt.locale(), "f", decimals)
        }

        implicitWidth: Math.max(metrics.advanceWidth(format(minimum)), metrics.advanceWidth(format(maximum)), metrics.advanceWidth(format(middle))) + 12
        FontMetrics {
            id: metrics
            font: Theme.font
        }
        Repeater {
            model: [axis.maximum, axis.middle, axis.minimum]
            delegate: Item {
                required property real modelData
                width: axis.width
                height: 1
                y: (axis.maximum - modelData) / (axis.maximum - axis.minimum) * axis.height
                Rectangle {
                    id: tick
                    anchors.right: parent.right
                    width: 5
                    height: 1
                    color: Theme.borderColor
                }
                Label {
                    anchors.right: tick.left
                    anchors.rightMargin: 4
                    anchors.verticalCenter: tick.verticalCenter
                    text: axis.format(modelData)
                    color: Theme.foregroundSecondaryColor
                }
            }
        }
    }

    GroupBox {
        Layout.fillWidth: true
        title: qsTr("Tempo")
        contentItem: ColumnLayout {
            spacing: 12

            FontMetrics {
                id: tempoMetrics
                font: Theme.font
            }
            FontMetrics {
                id: automaticBpmMetrics
                font: automaticBpmText.font
            }
            GridLayout {
                id: tempoSettings
                readonly property real bpmFieldWidth: Math.max(120, tempoMetrics.advanceWidth((1000).toLocaleString(Qt.locale(), "f", 2)) + 48,
                                                              automaticBpmMetrics.advanceWidth((1000).toLocaleString(Qt.locale(), "f", 2)))
                readonly property real segmentFieldWidth: Math.max(100, tempoMetrics.advanceWidth((64).toLocaleString(Qt.locale(), "f", 0)) + 48)
                readonly property real fixedMinimumWidth: bpmFieldWidth + automaticBpmBox.implicitWidth + tempoScaleButtons.implicitWidth + 16
                readonly property real variableMinimumWidth: Math.max(smoothingSlider.implicitWidth,
                                                                    smoothingLabel.implicitWidth + tempoMetrics.advanceWidth((100).toLocaleString(Qt.locale(), "f", 0)) + 8,
                                                                    variationLabel.implicitWidth + smootherLabel.implicitWidth + 8)
                                                           + Math.max(segmentLabel.implicitWidth, segmentFieldWidth) + 16
                readonly property real editorMinimumWidth: Math.max(fixedMinimumWidth, variableMinimumWidth)
                readonly property bool stacked: width < modeControls.implicitWidth + editorMinimumWidth + 2 * columnSpacing + 1
                readonly property bool wrapEditors: width < editorMinimumWidth
                Layout.fillWidth: true
                columns: stacked ? 1 : 3
                columnSpacing: 16
                rowSpacing: 12

                ColumnLayout {
                    id: modeControls
                    Layout.minimumWidth: implicitWidth
                    Layout.maximumWidth: implicitWidth
                    Layout.alignment: Qt.AlignVCenter
                    spacing: 8
                    Label {
                        id: modeLabel
                        text: qsTr("Mode")
                    }
                    TabBar {
                        id: modeBar
                        Layout.minimumWidth: implicitWidth
                        Layout.maximumWidth: implicitWidth
                        implicitWidth: fixedModeButton.implicitWidth + variableModeButton.implicitWidth + spacing + leftPadding + rightPadding
                        currentIndex: view.preview.variable ? 1 : 0
                        onCurrentIndexChanged: {
                            if ((activeFocus || fixedModeButton.activeFocus || variableModeButton.activeFocus) && currentIndex >= 0 && currentIndex < 2)
                                view.preview.variable = currentIndex === 1
                        }
                        Accessible.name: modeLabel.text
                        Accessible.labelledBy: modeLabel
                        TabButton {
                            id: fixedModeButton
                            width: implicitWidth
                            text: qsTr("Fixed")
                            onClicked: view.preview.variable = false
                        }
                        TabButton {
                            id: variableModeButton
                            width: implicitWidth
                            text: qsTr("Variable")
                            onClicked: view.preview.variable = true
                        }
                    }
                }
                Rectangle {
                    Layout.preferredWidth: 1
                    Layout.fillHeight: true
                    visible: !tempoSettings.stacked
                    color: Theme.borderColor
                }
                StackLayout {
                    id: tempoPages
                    Layout.fillWidth: true
                    Layout.minimumHeight: implicitHeight
                    Layout.maximumHeight: implicitHeight
                    implicitHeight: Math.max(fixedTempoPage.implicitHeight, variableTempoPage.implicitHeight)
                    currentIndex: view.preview.variable ? 1 : 0

                    Item {
                        id: fixedTempoPage
                        implicitHeight: fixedSettings.implicitHeight
                        GridLayout {
                            id: fixedSettings
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.verticalCenter: parent.verticalCenter
                            columns: tempoSettings.wrapEditors ? 2 : 3
                            columnSpacing: 8
                            rowSpacing: 8
                            Label {
                                id: bpmLabel
                                Layout.columnSpan: fixedSettings.columns
                                text: qsTr("Tempo (BPM)")
                            }
                            Item {
                                Layout.fillWidth: true
                                Layout.minimumWidth: tempoSettings.bpmFieldWidth
                                implicitWidth: Math.max(fixedBpmInput.implicitWidth, automaticBpmText.implicitWidth)
                                implicitHeight: Math.max(fixedBpmInput.implicitHeight, automaticBpmText.implicitHeight)
                                Label {
                                    id: automaticBpmText
                                    anchors.fill: parent
                                    visible: view.preview.automatic
                                    font.pixelSize: Math.max(1, Math.round(fixedBpmInput.implicitHeight * 1.4))
                                    font.bold: true
                                    verticalAlignment: Text.AlignVCenter
                                    text: view.preview.bpm.toLocaleString(Qt.locale(), "f", 2)
                                    Accessible.labelledBy: bpmLabel
                                }
                                DoubleSpinBox {
                                    id: fixedBpmInput
                                    anchors.left: parent.left
                                    anchors.right: parent.right
                                    anchors.verticalCenter: parent.verticalCenter
                                    visible: !view.preview.automatic
                                    from: 10
                                    to: 1000
                                    decimals: 2
                                    value: view.preview.bpm
                                    onValueModified: view.preview.bpm = value
                                    Accessible.name: bpmLabel.text
                                    Accessible.labelledBy: bpmLabel
                                }
                            }
                            CheckBox {
                                id: automaticBpmBox
                                text: qsTr("Automatic BPM")
                                checked: view.preview.automatic
                                onToggled: view.preview.automatic = checked
                            }
                            RowLayout {
                                id: tempoScaleButtons
                                Layout.columnSpan: tempoSettings.wrapEditors ? 2 : 1
                                Layout.alignment: Qt.AlignRight
                                spacing: 8
                                Button {
                                    text: qsTr("÷ 2")
                                    enabled: view.preview.valid && view.preview.bpm >= 20
                                    onClicked: view.preview.scaleBpm(0.5)
                                    Accessible.name: qsTr("Halve Tempo")
                                }
                                Button {
                                    text: qsTr("× 2")
                                    enabled: view.preview.valid && view.preview.bpm <= 500
                                    onClicked: view.preview.scaleBpm(2)
                                    Accessible.name: qsTr("Double Tempo")
                                }
                            }
                        }
                    }
                    Item {
                        id: variableTempoPage
                        implicitHeight: variableSettings.implicitHeight
                        GridLayout {
                            id: variableSettings
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.verticalCenter: parent.verticalCenter
                            columns: tempoSettings.wrapEditors ? 1 : 2
                            columnSpacing: 16
                            rowSpacing: 8
                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 8
                                RowLayout {
                                    Layout.fillWidth: true
                                    Label {
                                        id: smoothingLabel
                                        Layout.fillWidth: true
                                        text: qsTr("Smoothing")
                                    }
                                    Label {
                                        id: smoothingValue
                                        Layout.minimumWidth: tempoMetrics.advanceWidth((100).toLocaleString(Qt.locale(), "f", 0))
                                        horizontalAlignment: Text.AlignRight
                                        text: Math.round(view.preview.smoothing).toLocaleString(Qt.locale(), "f", 0)
                                    }
                                }
                                Slider {
                                    id: smoothingSlider
                                    Layout.fillWidth: true
                                    from: 0
                                    to: 100
                                    stepSize: 1
                                    snapMode: Slider.SnapAlways
                                    value: view.preview.smoothing
                                    onMoved: view.preview.smoothing = Math.round(value)
                                    Accessible.name: smoothingLabel.text
                                    Accessible.labelledBy: smoothingLabel
                                }
                                RowLayout {
                                    Layout.fillWidth: true
                                    Label {
                                        id: variationLabel
                                        Layout.fillWidth: true
                                        text: qsTr("More variation")
                                        color: Theme.foregroundSecondaryColor
                                    }
                                    Label {
                                        id: smootherLabel
                                        text: qsTr("Smoother")
                                        color: Theme.foregroundSecondaryColor
                                    }
                                }
                            }
                            ColumnLayout {
                                Layout.fillWidth: tempoSettings.wrapEditors
                                Layout.alignment: Qt.AlignTop
                                spacing: 8
                                Label {
                                    id: segmentLabel
                                    text: qsTr("Minimum segment (beats)")
                                }
                                SpinBox {
                                    Layout.fillWidth: true
                                    Layout.minimumWidth: tempoSettings.segmentFieldWidth
                                    from: 1
                                    to: 64
                                    value: view.preview.minimumSegmentBeats
                                    onValueModified: view.preview.minimumSegmentBeats = value
                                    Accessible.name: segmentLabel.text
                                    Accessible.labelledBy: segmentLabel
                                }
                            }
                        }
                    }
                }
            }
            GridLayout {
                Layout.fillWidth: true
                columns: 2
                columnSpacing: 12
                rowSpacing: 8
                Label {
                    id: alignmentLabel
                    Layout.row: 0
                    Layout.column: 0
                    Layout.minimumWidth: view.firstLabelWidth
                    Layout.maximumWidth: view.firstLabelWidth
                    text: qsTr("Alignment")
                }
                ComboBox {
                    Layout.row: 0
                    Layout.column: 1
                    Layout.fillWidth: true
                    Layout.preferredWidth: 1
                    model: view.preview.canKeepAudioPosition ? [qsTr("Move Audio"), qsTr("Keep Audio Position")] : [qsTr("Move Audio")]
                    currentIndex: view.preview.canKeepAudioPosition && !view.preview.moveAudio ? 1 : 0
                    onActivated: view.preview.moveAudio = currentIndex === 0
                    Accessible.name: alignmentLabel.text
                    Accessible.labelledBy: alignmentLabel
                }
                Label {
                    Layout.row: 1
                    Layout.column: 1
                    Layout.fillWidth: true
                    visible: text.length > 0
                    text: view.preview.alignmentHint
                    color: Theme.foregroundSecondaryColor
                    wrapMode: Text.Wrap
                }
            }
        }
    }

    GroupBox {
        Layout.fillWidth: true
        title: qsTr("Beat Assignment")
        contentItem: GridLayout {
            columns: view.compactSettings ? 2 : 4
            columnSpacing: 12
            rowSpacing: 8
            Label {
                id: referenceLabel
                Layout.minimumWidth: view.firstLabelWidth
                Layout.maximumWidth: view.firstLabelWidth
                text: qsTr("Detected beat")
            }
            ComboBox {
                id: referenceBox
                Layout.fillWidth: true
                Layout.preferredWidth: 1
                model: view.preview.beatLabels
                currentIndex: view.preview.referenceBeat
                onActivated: view.preview.referenceBeat = currentIndex
                Accessible.name: referenceLabel.text
                Accessible.labelledBy: referenceLabel
            }
            Label {
                id: positionLabel
                Layout.minimumWidth: view.compactSettings ? view.firstLabelWidth : view.secondLabelWidth
                Layout.maximumWidth: view.compactSettings ? view.firstLabelWidth : view.secondLabelWidth
                text: qsTr("Assign to position")
            }
            MusicTimeSpinBox {
                Layout.fillWidth: true
                Layout.preferredWidth: 1
                timeline: view.preview.timeline
                value: view.preview.referenceTick
                onValueModified: view.preview.referenceTick = value
                Accessible.name: positionLabel.text
                Accessible.labelledBy: positionLabel
            }
        }
    }

    Label {
        Layout.fillWidth: true
        visible: text.length > 0
        text: view.preview.error
        color: Theme.errorColor
        wrapMode: Text.Wrap
    }

    GridLayout {
        id: charts
        readonly property real axisWidth: Math.ceil(Math.max(tempoAxis.implicitWidth, errorAxis.implicitWidth, zoomLabel.implicitWidth))
        Layout.fillWidth: true
        columns: 2
        rows: 8
        columnSpacing: 0
        rowSpacing: 8

        GridLayout {
            Layout.row: 0
            Layout.column: 1
            Layout.fillWidth: true
            columns: view.compactSettings ? 1 : 2
            Label {
                Layout.fillWidth: true
                text: qsTr("Waveform")
            }
            CheckBox {
                id: candidateBox
                text: qsTr("Preview Beats")
                checked: true
            }
        }

        Frame {
            id: waveformFrame
            Layout.row: 1
            Layout.column: 1
            Layout.fillWidth: true
            Layout.preferredHeight: 104
            padding: 0
            clip: true
            focusPolicy: Qt.StrongFocus
            Accessible.role: Accessible.Slider
            Accessible.name: qsTr("Audio Preview Position")
            Accessible.onIncreaseAction: view.preview.seek(view.preview.playhead + 1)
            Accessible.onDecreaseAction: view.preview.seek(view.preview.playhead - 1)
            Keys.onLeftPressed: view.preview.seek(view.preview.playhead - 1)
            Keys.onRightPressed: view.preview.seek(view.preview.playhead + 1)
            Keys.onPressed: event => {
                if (event.key === Qt.Key_Home) {
                    view.preview.seek(view.preview.viewStart)
                    event.accepted = true
                } else if (event.key === Qt.Key_End) {
                    view.preview.seek(view.preview.viewEnd)
                    event.accepted = true
                } else {
                    event.accepted = false
                }
            }
            WaveformThumbnail {
                anchors.fill: parent
                waveformMipmap: view.preview.waveform
                waveformOffset: (view.preview.viewStart - view.preview.begin) * view.preview.waveformRate
                color: Theme.foregroundSecondaryColor
                waveformSections: [{
                    start: 0,
                    end: 1,
                    length: (view.preview.viewEnd - view.preview.viewStart) * view.preview.waveformRate
                }]
            }
            TempoExtractionPlot {
                anchors.fill: parent
                preview: view.preview
                plot: TempoExtractionPlot.Beats
                candidateVisible: candidateBox.checked
                color: Qt.rgba(Theme.accentColor.r, Theme.accentColor.g, Theme.accentColor.b, Theme.accentColor.a * 0.45)
                detectedColor: Theme.foregroundPrimaryColor
            }
            Playhead {
                preview: view.preview
            }
            MouseArea {
                anchors.fill: parent
                onPressed: waveformFrame.forceActiveFocus()
                onClicked: mouse => {
                    const seconds = view.preview.viewStart + mouse.x / width * (view.preview.viewEnd - view.preview.viewStart)
                    if (mouse.y < 24)
                        view.preview.selectBeat(seconds, 8 / width * (view.preview.viewEnd - view.preview.viewStart))
                    else
                        view.preview.seek(seconds)
                }
            }
        }

        RowLayout {
            Layout.row: 2
            Layout.column: 1
            Layout.fillWidth: true
            spacing: 8
            Label {
                text: qsTr("Tempo (BPM)")
            }
            Label {
                Layout.fillWidth: true
                text: view.preview.tempoChangeCount > 0 ? qsTr("Changes: %1").arg(view.preview.tempoChangeCount.toLocaleString(Qt.locale(), "f", 0)) : ""
                color: Theme.foregroundSecondaryColor
                elide: Text.ElideRight
            }
            Label {
                readonly property real bpm: view.preview.tempoChangeCount > 0 && tempoHover.hovered
                    ? view.preview.tempoAt(view.preview.viewStart + Math.min(1, Math.max(0, tempoHover.point.position.x / Math.max(1, tempoPlot.width)))
                                           * (view.preview.viewEnd - view.preview.viewStart)) : 0
                Layout.alignment: Qt.AlignRight
                text: bpm > 0 ? qsTr("%1 BPM").arg(bpm.toLocaleString(Qt.locale(), "f", 2)) : ""
                horizontalAlignment: Text.AlignRight
            }
        }
        ValueAxis {
            id: tempoAxis
            Layout.row: 3
            Layout.column: 0
            Layout.minimumWidth: charts.axisWidth
            Layout.maximumWidth: charts.axisWidth
            Layout.fillHeight: true
            minimum: tempoPlot.minimum
            maximum: tempoPlot.maximum
            decimals: 1
        }
        Frame {
            Layout.row: 3
            Layout.column: 1
            Layout.fillWidth: true
            Layout.preferredHeight: 80
            padding: 0
            clip: true
            TempoExtractionPlot {
                id: tempoPlot
                anchors.fill: parent
                preview: view.preview
                plot: TempoExtractionPlot.Tempo
                color: Theme.accentColor
                Accessible.role: Accessible.Graphic
                Accessible.name: qsTr("Extracted Tempo")
                HoverHandler {
                    id: tempoHover
                }
            }
            Playhead {
                preview: view.preview
            }
        }

        RowLayout {
            Layout.row: 4
            Layout.column: 1
            Layout.fillWidth: true
            spacing: 8
            Label {
                text: errorAxis.valueScale === 1000 ? qsTr("Beat offset (s)") : qsTr("Beat offset (ms)")
            }
            Label {
                Layout.fillWidth: true
                text: view.preview.tempoChangeCount > 0 ? qsTr("Maximum: %1 ms").arg(view.preview.maximumOffsetMsec.toLocaleString(Qt.locale(), "f", 1)) : ""
                color: Theme.foregroundSecondaryColor
                elide: Text.ElideRight
            }
        }
        ValueAxis {
            id: errorAxis
            Layout.row: 5
            Layout.column: 0
            Layout.minimumWidth: charts.axisWidth
            Layout.maximumWidth: charts.axisWidth
            Layout.fillHeight: true
            minimum: errorPlot.minimum
            maximum: errorPlot.maximum
            valueScale: Math.max(Math.abs(minimum), Math.abs(maximum)) >= 1000 ? 1000 : 1
            decimals: valueScale === 1000 ? 1 : 0
        }
        Frame {
            Layout.row: 5
            Layout.column: 1
            Layout.fillWidth: true
            Layout.preferredHeight: 80
            padding: 0
            clip: true
            TempoExtractionPlot {
                id: errorPlot
                anchors.fill: parent
                preview: view.preview
                plot: TempoExtractionPlot.Deviation
                color: Theme.accentColor
                gridColor: Theme.borderColor
                Accessible.role: Accessible.Graphic
                Accessible.name: qsTr("Beat Deviation")
            }
            Playhead {
                preview: view.preview
            }
        }

        RowLayout {
            Layout.row: 6
            Layout.column: 1
            Layout.fillWidth: true
            Label {
                text: view.preview.formatTime(Math.round(view.preview.viewStart * 1000))
            }
            Item {
                Layout.fillWidth: true
            }
            Label {
                text: view.preview.formatTime(Math.round(view.preview.viewEnd * 1000))
            }
        }

        Label {
            id: zoomLabel
            Layout.row: 7
            Layout.column: 0
            text: qsTr("Zoom")
        }
        RangeSlider {
            Layout.row: 7
            Layout.column: 1
            Layout.fillWidth: true
            from: view.preview.begin
            to: view.preview.end
            stepSize: 0.001
            first.value: view.preview.viewStart
            second.value: view.preview.viewEnd
            first.onMoved: view.preview.setRange(first.value, second.value)
            second.onMoved: view.preview.setRange(first.value, second.value)
            Accessible.name: qsTr("Visible Time Range")
        }
    }

    GridLayout {
        Layout.fillWidth: true
        columns: view.compactSettings ? 2 : 6
        columnSpacing: 8
        rowSpacing: 8
        Label {
            id: startLabel
            text: qsTr("View start")
        }
        SpinBox {
            Layout.fillWidth: view.compactSettings
            from: Math.round(view.preview.begin * 1000)
            to: Math.round(view.preview.viewEnd * 1000) - 1
            value: Math.round(view.preview.viewStart * 1000)
            validator: view.preview.startTimeValidator
            textFromValue: value => view.preview.formatTime(value)
            valueFromText: text => view.preview.parseTime(text, value)
            onValueModified: view.preview.setRange(value / 1000, view.preview.viewEnd)
            Accessible.name: startLabel.text
            Accessible.labelledBy: startLabel
        }
        Label {
            id: endLabel
            text: qsTr("View end")
        }
        SpinBox {
            Layout.fillWidth: view.compactSettings
            from: Math.round(view.preview.viewStart * 1000) + 1
            to: Math.round(view.preview.end * 1000)
            value: Math.round(view.preview.viewEnd * 1000)
            validator: view.preview.endTimeValidator
            textFromValue: value => view.preview.formatTime(value)
            valueFromText: text => view.preview.parseTime(text, value)
            onValueModified: view.preview.setRange(view.preview.viewStart, value / 1000)
            Accessible.name: endLabel.text
            Accessible.labelledBy: endLabel
        }
        Button {
            Layout.columnSpan: view.compactSettings ? 2 : 1
            Layout.alignment: view.compactSettings ? Qt.AlignRight : Qt.AlignLeft
            text: qsTr("Fit All")
            onClicked: view.preview.setRange(view.preview.begin, view.preview.end)
        }
        Item {
            Layout.fillWidth: true
            visible: !view.compactSettings
        }
    }

    GridLayout {
        Layout.fillWidth: true
        columns: view.compactSettings ? 3 : 5
        columnSpacing: 8
        rowSpacing: 8
        Button {
            text: view.preview.playing ? qsTr("Pause") : qsTr("Play")
            enabled: view.preview.valid
            onClicked: view.preview.togglePlayback()
        }
        CheckBox {
            text: qsTr("Metronome")
            checked: view.preview.metronome
            onToggled: view.preview.metronome = checked
        }
        CheckBox {
            text: qsTr("Loop")
            checked: view.preview.loop
            onToggled: view.preview.loop = checked
        }
        Item {
            Layout.fillWidth: true
            visible: !view.compactSettings
        }
        SpinBox {
            Layout.columnSpan: view.compactSettings ? 3 : 1
            Layout.alignment: Qt.AlignRight
            from: Math.round(view.preview.viewStart * 1000)
            to: Math.round(view.preview.viewEnd * 1000)
            value: Math.round(view.preview.playhead * 1000)
            validator: view.preview.timeValidator
            textFromValue: value => view.preview.formatTime(value)
            valueFromText: text => view.preview.parseTime(text, value)
            onValueModified: view.preview.seek(value / 1000)
            Accessible.name: qsTr("Playback Position")
        }
    }

}
