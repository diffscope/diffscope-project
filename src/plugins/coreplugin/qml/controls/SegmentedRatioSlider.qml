import QtQml
import QtQuick
import QtQuick.Controls

import SVSCraft
import SVSCraft.UIComponents

Item {
    id: control

    property var names: []
    property var values: []

    signal editingStarted()
    signal adjacentRatioModified(int leftIndex, real leftRatio)
    signal editingFinished()
    signal editingCanceled()

    implicitHeight: 52

    readonly property int segmentCount: Math.min(names?.length ?? 0,
                                                  values?.length ?? 0)
    readonly property var percentageValues: {
        let result = []
        let order = []
        let used = 0
        for (let i = 0; i < segmentCount; ++i) {
            const exact = Number(values[i] ?? 0) * 1000
            const base = Math.floor(exact)
            result.push(base)
            used += base
            order.push({ index: i, fraction: exact - base })
        }
        order.sort((a, b) => b.fraction - a.fraction || a.index - b.index)
        for (let i = 0; i < 1000 - used && i < order.length; ++i)
            result[order[i].index] += 1
        for (let i = 0; i < result.length; ++i)
            result[i] /= 10
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

        Rectangle {
            anchors.fill: parent
            radius: 6
            color: Theme.borderColor
        }

        Item {
            anchors.fill: parent
            anchors.margins: 1
            clip: true

            Repeater {
                model: control.segmentCount

                delegate: Rectangle {
                    id: segment

                    required property int index

                    x: control.cumulative(index) * parent.width
                    y: 0
                    width: index === control.segmentCount - 1
                           ? Math.max(0, parent.width - x)
                           : Math.max(0, Number(control.values[index] ?? 0) * parent.width)
                    height: parent.height
                    color: Theme.buttonColor

                    Column {
                        anchors.centerIn: parent
                        width: Math.max(0, parent.width - 8)
                        spacing: 0

                        Label {
                            width: parent.width
                            horizontalAlignment: Text.AlignHCenter
                            elide: Text.ElideRight
                            color: Theme.foregroundPrimaryColor
                            text: String(control.names[segment.index] ?? "")
                        }

                        Label {
                            width: parent.width
                            horizontalAlignment: Text.AlignHCenter
                            color: Theme.foregroundPrimaryColor
                            text: qsTr("%1%").arg(Number(
                                control.percentageValues[segment.index] ?? 0).toLocaleString(
                                    Qt.locale(), "f", 1))
                        }
                    }
                }
            }
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
                       ? Theme.accentColor : Theme.borderColor
            }

            MouseArea {
                id: handleMouseArea
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.SizeHorCursor

                onPressed: mouse => {
                    handle.pressX = mapToItem(control, mouse.x, mouse.y).x
                    handle.initialLeft = Number(control.values[handle.index] ?? 0)
                    control.editingStarted()
                }
                onPositionChanged: mouse => {
                    if (!pressed || control.width <= 0)
                        return
                    const currentX = mapToItem(control, mouse.x, mouse.y).x
                    const nextLeft = handle.initialLeft
                                     + (currentX - handle.pressX) / control.width
                    control.adjacentRatioModified(handle.index, nextLeft)
                }
                onReleased: control.editingFinished()
                onCanceled: control.editingCanceled()
            }
        }
    }
}
