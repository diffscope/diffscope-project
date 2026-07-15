import QtQml
import QtQml.Models
import QtQuick
import QtQuick.Controls

import SVSCraft
import SVSCraft.UIComponents

import DiffScope.Core

Item {
    id: control

    required property SourcesPickerModel sourcesModel
    required property var mixedSingerIndex

    implicitHeight: 52

    readonly property bool mixedSingerIndexValid: {
        sourcesModel.revision
        return Boolean(mixedSingerIndex && sourcesModel.indexAlive(mixedSingerIndex))
    }
    readonly property int segmentCount: {
        return mixedSingerIndexValid ? segmentDelegateModel.count : 0
    }
    readonly property var values: {
        const currentRevision = sourcesModel.revision
        const currentCount = segmentDelegateModel.count
        return mixedSingerIndexValid && currentRevision >= 0 && currentCount > 0
               ? sourcesModel.ratios(mixedSingerIndex)
               : []
    }
    readonly property var percentageValues: {
        let result = []
        let order = []
        let used = 0
        for (let i = 0; i < values.length; ++i) {
            const exact = Number(values[i]) * 100
            const base = Math.floor(exact)
            result.push(base)
            used += base
            order.push({ index: i, fraction: exact - base })
        }
        order.sort((a, b) => b.fraction - a.fraction || a.index - b.index)
        for (let i = 0; i < 100 - used && i < order.length; ++i)
            result[order[i].index] += 1
        return result
    }

    function cumulative(index) {
        let result = 0
        for (let i = 0; i < index; ++i)
            result += Number(values[i] ?? 0)
        return result
    }

    Item {
        id: track
        anchors.fill: parent
        anchors.topMargin: 8
        anchors.bottomMargin: 8
        clip: true

        DelegateModel {
            id: segmentDelegateModel

            model: control.sourcesModel
            rootIndex: control.mixedSingerIndex

            delegate: Rectangle {
                id: segment

                required property int index
                required property string displayName

                x: control.cumulative(index) * track.width
                y: 0
                width: index === control.values.length - 1
                       ? Math.max(0, track.width - x)
                       : Math.max(0, Number(control.values[index] ?? 0) * track.width)
                height: track.height
                color: Theme.buttonColor
                topLeftRadius: index === 0 ? 6 : 0
                bottomLeftRadius: topLeftRadius
                topRightRadius: index === control.values.length - 1 ? 6 : 0
                bottomRightRadius: topRightRadius

                Label {
                    anchors.centerIn: parent
                    width: Math.max(0, parent.width - 8)
                    horizontalAlignment: Text.AlignHCenter
                    elide: Text.ElideRight
                    color: Theme.foregroundPrimaryColor
                    text: qsTr("%1  %2%").arg(segment.displayName)
                                             .arg(Number(control.percentageValues[segment.index] ?? 0))
                }
            }
        }

        Repeater {
            model: segmentDelegateModel
        }

        Rectangle {
            anchors.fill: parent
            color: "transparent"
            border.width: 1
            border.color: Theme.borderColor
            radius: 6
        }
    }

    Repeater {
        model: Math.max(0, control.segmentCount - 1)

        delegate: Item {
            id: handle

            required property int index

            property real pressX
            property real initialLeft

            x: control.cumulative(index + 1) * control.width - width / 2
            y: 0
            width: 16
            height: control.height
            z: 2

            Rectangle {
                anchors.centerIn: parent
                width: 2
                height: track.height + 4
                radius: 1
                color: handleMouseArea.containsMouse || handleMouseArea.pressed
                       ? Theme.accentColor
                       : Theme.borderColor
            }

            MouseArea {
                id: handleMouseArea
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.SizeHorCursor

                onPressed: mouse => {
                    handle.pressX = mapToItem(control, mouse.x, mouse.y).x
                    handle.initialLeft = Number(control.values[handle.index])
                }
                onPositionChanged: mouse => {
                    if (!pressed || control.width <= 0)
                        return
                    const currentX = mapToItem(control, mouse.x, mouse.y).x
                    const nextLeft = handle.initialLeft + (currentX - handle.pressX) / control.width
                    control.sourcesModel.setAdjacentRatios(control.mixedSingerIndex, handle.index, nextLeft)
                }
            }
        }
    }
}
