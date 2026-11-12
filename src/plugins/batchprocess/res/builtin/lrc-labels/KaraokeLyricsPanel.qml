import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SVSCraft
import SVSCraft.UIComponents

import DiffScope.BatchProcess
import DiffScope.DspxModel as DspxModel
import DiffScope.DspxProject as DspxProject

ScriptPanel {
    id: scriptPanel

    component LyricText: Text {
        textFormat: Text.PlainText
        padding: 2
        style: Text.Outline
        styleColor: Theme.foregroundPrimaryColor
    }

    Component {
        DockingPane {
            id: pane

            property int lyricPixelSize: 48

            leftPadding: 24
            rightPadding: 24

            header: Item {
                anchors.fill: parent

                RowLayout {
                    anchors.fill: parent

                    ToolButton {
                        icon.source: "image://fluent-system-icons/add_circle"
                        text: scriptPanel.scriptContext.t("lrcLabels.karaoke.addTitle")
                        display: AbstractButton.IconOnly
                        enabled: lyrics.timeline !== null && !lyrics.hasLabelAtPlayhead
                        onClicked: scriptPanel.scriptContext.invokeMethod("karaoke.editLyrics", {
                            add: true,
                            position: lyrics.timeline.position
                        })
                    }

                    ToolButton {
                        icon.source: "image://fluent-system-icons/edit"
                        text: scriptPanel.scriptContext.t("lrcLabels.karaoke.editTitle")
                        display: AbstractButton.IconOnly
                        enabled: lyrics.currentLabel !== null
                        onClicked: scriptPanel.scriptContext.invokeMethod("karaoke.editLyrics", {
                            add: false,
                            position: lyrics.currentLabel.position
                        })
                    }

                    Item {
                        Layout.fillWidth: true
                    }

                    ToolButton {
                        icon.source: "image://fluent-system-icons/text_font_size"
                        text: scriptPanel.scriptContext.t("lrcLabels.karaoke.fontSizeTitle")
                        display: AbstractButton.IconOnly
                        onClicked: {
                            const fontSize = scriptPanel.scriptContext.invokeMethod("karaoke.editFontSize")
                            if (typeof fontSize === "number")
                                pane.lyricPixelSize = fontSize
                        }
                    }
                }
            }

            onVisibleChanged: lyrics.requestUpdate()
            Component.onCompleted: {
                lyrics.update()
                const fontSize = scriptPanel.scriptContext.invokeMethod("karaoke.fontSize")
                if (typeof fontSize === "number")
                    pane.lyricPixelSize = fontSize
            }

            QtObject {
                id: lyrics

                readonly property DspxProject.ProjectTimeline timeline: scriptPanel.windowHandle?.projectTimeline ?? null
                readonly property DspxModel.LabelSequence labels: scriptPanel.windowHandle?.projectDocumentContext?.document?.model?.labels ?? null
                readonly property MusicTimeline musicTimeline: timeline?.musicTimeline ?? null
                readonly property bool hasLabels: (labels?.size ?? 0) > 0
                readonly property string line: (currentLabel?.text ?? "").replace(/[\r\n\u2028\u2029]+/g, " ")
                readonly property bool hasText: line.trim().length > 0
                readonly property bool animated: currentLabel !== null && !terminal
                readonly property string displayText: {
                    if (!currentLabel || (terminal && !hasText))
                        return ""
                    return hasText ? line : "♫♫♫♫"
                }

                property var entries: []
                property bool indexDirty: true
                property int currentIndex: -1
                property DspxModel.Label currentLabel: null
                property bool hasLabelAtPlayhead: false
                property bool terminal: false
                property real fillProgress: 0

                function requestUpdate() {
                    if (pane.visible && !frameUpdate.running)
                        frameUpdate.start()
                }

                function invalidateIndex() {
                    lyrics.indexDirty = true
                    lyrics.requestUpdate()
                }

                function rebuildIndex() {
                    const previouslyIndexed = new Set(entries.map(entry => entry.label))
                    const orderedEntries = []
                    for (let label = labels?.firstItem; label; label = label.nextItem) {
                        if (!previouslyIndexed.delete(label))
                            label.positionChanged.connect(lyrics.invalidateIndex)
                        orderedEntries.push({ label, position: label.position })
                    }
                    for (const label of previouslyIndexed)
                        label?.positionChanged?.disconnect(lyrics.invalidateIndex)
                    entries = orderedEntries
                    currentIndex = -1
                    indexDirty = false
                }

                function indexAt(position) {
                    let lower = 0
                    let upper = entries.length
                    while (lower < upper) {
                        const middle = Math.floor((lower + upper) / 2)
                        if (entries[middle].position <= position)
                            lower = middle + 1
                        else
                            upper = middle
                    }
                    return lower - 1
                }

                function update() {
                    if (indexDirty)
                        rebuildIndex()

                    const position = timeline?.position ?? 0
                    const current = entries[currentIndex]
                    const next = entries[currentIndex + 1]
                    if ((current && position < current.position) || (next && position >= next.position))
                        currentIndex = indexAt(position)

                    const entry = entries[currentIndex]
                    const following = entries[currentIndex + 1]
                    currentLabel = entry?.label ?? null
                    hasLabelAtPlayhead = entry?.position === position
                    terminal = !!entry && !following
                    if (!animated || !musicTimeline) {
                        fillProgress = 0
                        return
                    }

                    const start = musicTimeline.create(0, 0, entry.position).millisecond
                    const end = musicTimeline.create(0, 0, following.position).millisecond
                    const elapsed = musicTimeline.create(0, 0, position).millisecond - start
                    const progress = Math.max(0, Math.min(1, elapsed / (end - start)))
                    fillProgress = progress * (2 - progress)
                }

                Component.onDestruction: {
                    for (const entry of entries)
                        entry.label?.positionChanged?.disconnect(lyrics.invalidateIndex)
                }
            }

            Connections {
                target: lyrics
                function onLabelsChanged() { lyrics.invalidateIndex() }
                function onTimelineChanged() { lyrics.requestUpdate() }
            }

            Connections {
                target: lyrics.labels
                function onItemInserted() { lyrics.invalidateIndex() }
                function onItemRemoved() { lyrics.invalidateIndex() }
            }

            Connections {
                target: lyrics.currentLabel
                function onTextChanged() { lyrics.requestUpdate() }
            }

            Connections {
                target: lyrics.timeline
                function onPositionChanged() { lyrics.requestUpdate() }
            }

            Connections {
                target: lyrics.musicTimeline
                function onChanged() { lyrics.requestUpdate() }
            }

            FrameAnimation {
                id: frameUpdate

                onTriggered: {
                    if (pane.visible)
                        lyrics.update()
                    frameUpdate.stop()
                }
            }

            contentItem: Item {
                id: viewport

                implicitHeight: Math.max(lyricLine.height, emptyLabel.implicitHeight)
                clip: true

                Label {
                    id: emptyLabel

                    anchors.centerIn: parent
                    visible: !lyrics.hasLabels
                    text: scriptPanel.scriptContext.t("lrcLabels.karaoke.noLyrics")
                }

                Item {
                    id: lyricLine

                    readonly property real fillWidth: lyrics.animated ? width * lyrics.fillProgress : 0

                    width: foregroundText.implicitWidth
                    height: foregroundText.implicitHeight
                    visible: lyrics.hasLabels
                    anchors.verticalCenter: parent.verticalCenter
                    x: {
                        const overflow = width - viewport.width
                        if (overflow <= 0 || !lyrics.animated)
                            return -overflow / 2
                        return -Math.min(overflow, Math.max(0, fillWidth - viewport.width / 2))
                    }

                    Item {
                        x: lyricLine.fillWidth
                        width: lyricLine.width - x
                        height: lyricLine.height
                        clip: true

                        LyricText {
                            id: foregroundText
                            x: -parent.x
                            text: lyrics.displayText
                            font.pixelSize: pane.lyricPixelSize
                            color: Theme.foregroundPrimaryColor
                        }
                    }

                    Item {
                        width: lyricLine.fillWidth
                        height: lyricLine.height
                        visible: width > 0
                        clip: true

                        LyricText {
                            text: foregroundText.text
                            font: foregroundText.font
                            color: Theme.accentColor
                            Accessible.ignored: true
                        }
                    }
                }
            }
        }
    }
}
