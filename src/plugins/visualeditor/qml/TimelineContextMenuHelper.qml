import QtQml
import QtQml.Models
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SVSCraft
import SVSCraft.UIComponents

import dev.sjimo.ScopicFlow
import dev.sjimo.ScopicFlow.Views

import DiffScope.Core

EditTempoTimeSignatureScenario {
    id: helper

    shouldDialogPopupAtCursor: true

    required property Timeline timeline

    readonly property MusicTimeline musicTimeline: projectTimeline?.musicTimeline ?? null

    readonly property TimeManipulator timeManipulator: TimeManipulator {
        id: timeManipulator
        target: helper.timeline
        timeLayoutViewModel: helper.timeline.timeLayoutViewModel
        timeViewModel: helper.timeline.timeViewModel
    }

    readonly property LoggingCategory lcTimelineContextMenuHelper: LoggingCategory {
        id: lcTimelineContextMenuHelper
        name: "diffscope.visualeditor.qml.timelinecontextmenuhelper"
    }

    readonly property Component newTimeSignatureMenu: Menu {
        id: menu
        required property int position
        Action {
            text: helper.musicTimeline?.create(0, 0, menu.position).toString() ?? ""
            enabled: false
        }
        MenuSeparator {
        }
        Action {
            text: qsTr("Insert Time Signature")
            onTriggered: {
                Qt.callLater(() => helper.insertTimeSignatureAt(menu.position))
            }
        }
        MenuSeparator {
        }
        Action {
            text: qsTr("Move Playhead Here")
            onTriggered: {
                helper.projectTimeline.goTo(menu.position)
            }
        }
    }

    readonly property Component existingTimeSignatureMenu: Menu {
        id: menu
        required property int position
        Action {
            text: helper.musicTimeline.create(0, 0, menu.position).toString()
            enabled: false
        }
        MenuSeparator {
        }
        Action {
            text: qsTr("Edit Time Signature")
            onTriggered: {
                Qt.callLater(() => helper.modifyExistingTimeSignatureAt(menu.position))
            }
        }
        Action {
            text: qsTr("Remove Time Signature")
            enabled: helper.musicTimeline.create(0, 0, menu.position).measure !== 0
            onTriggered: () => {
                let measure = helper.musicTimeline.create(0, 0, menu.position).measure
                if (measure === 0)
                    return
                let transactionId = helper.document.transactionController.beginTransaction()
                if (!transactionId) {
                    console.error(lcTimelineContextMenuHelper, "Failed to remove time signature in exclusive transaction")
                    return
                }
                let currentTimeSignatures = helper.document.model.timeline.timeSignatures.slice(measure, 1)
                for (let item of currentTimeSignatures) {
                    helper.document.model.timeline.timeSignatures.removeItem(item)
                    helper.document.model.destroyItem(item)
                }
                helper.document.transactionController.commitTransaction(transactionId, "Remove time signature")
            }
        }
        MenuSeparator {
        }
        Action {
            text: qsTr("Move Playhead Here")
            onTriggered: {
                helper.projectTimeline.goTo(menu.position)
            }
        }
    }

    readonly property Connections controllerConnections: Connections {
        target: helper.timeline.timelineInteractionController
        function onContextMenuRequested(timeline_, position) {
            if (timeline_ !== helper.timeline)
                return
            position = timeManipulator.alignPosition(position, ScopicFlow.AO_Visible)
            let measure = helper.musicTimeline.create(0, 0, position).measure
            let isTimeSignatureExisting = helper.musicTimeline.nearestBarWithTimeSignatureTo(measure) === measure
            let menu = (isTimeSignatureExisting ? helper.existingTimeSignatureMenu : helper.newTimeSignatureMenu).createObject(helper.window, { position })
            menu.popup()
        }
    }
}