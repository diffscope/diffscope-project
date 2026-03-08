import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SVSCraft
import SVSCraft.UIComponents

import QActionKit

import dev.sjimo.ScopicFlow

ActionCollection {
    id: d

    required property QtObject addOn
    property ProjectViewModelContext projectViewModelContext: addOn?.windowHandle.ProjectViewModelContext.context ?? null
    property ArrangementPanelInterface arrangementPanelInterface: null
    property PianoRollPanelInterface pianoRollPanelInterface: null
    property MixerPanelInterface mixerPanelInterface: null

    readonly property QtObject horizontalManipulator: d.addOn?.activeEditingArea === 0 ? arrangementPanelTimeManipulator : d.addOn?.activeEditingArea === 1 ? pianoRollPanelTimeManipulator : mixerTrackListManipulator
    readonly property QtObject verticalManipulator: d.addOn?.activeEditingArea === 0 ? trackListManipulator : d.addOn?.activeEditingArea === 1 ? clavierManipulator : null

    readonly property WheelManipulator wheelManipulator: WheelManipulator {
        id: wheelManipulator
        onMoved: (x, y, isPhysicalWheel) => {
            if (d.horizontalManipulator)
                d.horizontalManipulator.moveViewBy(x, isPhysicalWheel)
            if (d.verticalManipulator)
                d.verticalManipulator.moveViewBy(y, isPhysicalWheel)
        }
        onZoomed: (ratioX, ratioY, x, y, isPhysicalWheel) => {
            if (d.horizontalManipulator)
                d.horizontalManipulator.zoomViewBy(ratioX, x, isPhysicalWheel)
            if (d.verticalManipulator)
                d.verticalManipulator.zoomViewBy(ratioY, y, isPhysicalWheel)
        }
    }

    readonly property TimeManipulator arrangementPanelTimeManipulator: TimeManipulator {
        id: arrangementPanelTimeManipulator
        timeViewModel: d.arrangementPanelInterface?.timeViewModel ?? null
        timeLayoutViewModel: d.arrangementPanelInterface?.timeLayoutViewModel ?? null
        target: d.arrangementPanelInterface?.arrangementView.centerEditArea ?? null
    }

    readonly property TimeManipulator pianoRollPanelTimeManipulator: TimeManipulator {
        id: pianoRollPanelTimeManipulator
        timeViewModel: d.pianoRollPanelInterface?.timeViewModel ?? null
        timeLayoutViewModel: d.pianoRollPanelInterface?.timeLayoutViewModel ?? null
        target: d.pianoRollPanelInterface?.pianoRollView.centerEditArea ?? null
    }

    readonly property TrackListManipulator trackListManipulator: TrackListManipulator {
        id: trackListManipulator
        trackListViewModel: d.projectViewModelContext?.trackListViewModel ?? null
        trackListLayoutViewModel: d.arrangementPanelInterface?.trackListLayoutViewModel ?? null
        target: d.arrangementPanelInterface?.arrangementView.centerEditArea ?? null
    }

    readonly property ClavierManipulator clavierManipulator: ClavierManipulator {
        id: clavierManipulator
        clavierViewModel: d.pianoRollPanelInterface?.clavierViewModel ?? null
        target: d.pianoRollPanelInterface?.pianoRollView.centerEditArea ?? null
    }

    readonly property TrackListManipulator mixerTrackListManipulator: TrackListManipulator {
        id: mixerTrackListManipulator
        trackListViewModel: d.projectViewModelContext?.trackListViewModel ?? null
        trackListLayoutViewModel: d.mixerPanelInterface?.trackListLayoutViewModel ?? null
        target: d.mixerPanelInterface?.mixerView.centerEditArea ?? null
    }

    function handleWheel(x, y, isZoom, isPage) {
        let item = d.horizontalManipulator.target
        wheelManipulator.handleWheel(
            Qt.point(x * 120, y * 120),
            Qt.point(0, 0),
            d.addOn.activeEditingArea === 0 ? Qt.point(item.width / 2, item.height / 2) : Qt.point(item.width / 2, item.height / 2),
            d.addOn.activeEditingArea === 0 ? Qt.size(item.width, item.height) : Qt.size(item.width, item.height),
            false,
            isZoom,
            isPage
        )
    }

    ActionItem {
        actionId: "org.diffscope.visualeditor.view.scrollUp"
        Action {
            enabled: Boolean(d.verticalManipulator)
            onTriggered: d.handleWheel(0, 1, false, false)
        }
    }

    ActionItem {
        actionId: "org.diffscope.visualeditor.view.scrollDown"
        Action {
            enabled: Boolean(d.verticalManipulator)
            onTriggered: d.handleWheel(0, -1, false, false)
        }
    }

    ActionItem {
        actionId: "org.diffscope.visualeditor.view.scrollLeft"
        Action {
            enabled: Boolean(d.horizontalManipulator)
            onTriggered: d.handleWheel(1, 0, false, false)
        }
    }

    ActionItem {
        actionId: "org.diffscope.visualeditor.view.scrollRight"
        Action {
            enabled: Boolean(d.horizontalManipulator)
            onTriggered: d.handleWheel(-1, 0, false, false)
        }
    }

    ActionItem {
        actionId: "org.diffscope.visualeditor.view.scrollUpPage"
        Action {
            enabled: Boolean(d.verticalManipulator)
            onTriggered: d.handleWheel(0, 1, false, true)
        }
    }

    ActionItem {
        actionId: "org.diffscope.visualeditor.view.scrollDownPage"
        Action {
            enabled: Boolean(d.verticalManipulator)
            onTriggered: d.handleWheel(0, -1, false, true)
        }
    }

    ActionItem {
        actionId: "org.diffscope.visualeditor.view.scrollLeftPage"
        Action {
            enabled: Boolean(d.horizontalManipulator)
            onTriggered: d.handleWheel(1, 0, false, true)
        }
    }

    ActionItem {
        actionId: "org.diffscope.visualeditor.view.scrollRightPage"
        Action {
            enabled: Boolean(d.horizontalManipulator)
            onTriggered: d.handleWheel(-1, 0, false, true)
        }
    }

    ActionItem {
        actionId: "org.diffscope.visualeditor.view.scrollToTop"
        Action {
            enabled: Boolean(d.verticalManipulator)
            onTriggered: d.verticalManipulator.moveToStart(true)
        }
    }

    ActionItem {
        actionId: "org.diffscope.visualeditor.view.scrollToBottom"
        Action {
            enabled: Boolean(d.verticalManipulator)
            onTriggered: d.verticalManipulator.moveToEnd(true)
        }
    }

    ActionItem {
        actionId: "org.diffscope.visualeditor.view.scrollToLeftmost"
        Action {
            enabled: Boolean(d.horizontalManipulator)
            onTriggered: d.horizontalManipulator.moveToStart(true)
        }
    }

    ActionItem {
        actionId: "org.diffscope.visualeditor.view.scrollToRightmost"
        Action {
            enabled: Boolean(d.horizontalManipulator)
            onTriggered: d.horizontalManipulator.moveToEnd(true)
        }
    }

    ActionItem {
        actionId: "org.diffscope.visualeditor.view.scrollToPlaybackPosition"
        Action {
            enabled: d.addOn?.activeEditingArea !== 2
            onTriggered: () => {
                let playbackPosition = d.addOn.windowHandle.projectTimeline.position
                d.horizontalManipulator.ensureVisible(playbackPosition, 0, d.horizontalManipulator.target.width / 2, d.horizontalManipulator.target.width / 2, true)
            }
        }
    }

    ActionItem {
        actionId: "org.diffscope.visualeditor.view.scrollToCurrentItem"
        Action {
            // TODO
        }
    }

    ActionItem {
        actionId: "org.diffscope.visualeditor.view.verticalZoomIn"
        Action {
            enabled: Boolean(d.verticalManipulator && d.verticalManipulator.zoomViewBy)
            onTriggered: d.handleWheel(0, 1, true, false)
        }
    }

    ActionItem {
        actionId: "org.diffscope.visualeditor.view.verticalZoomOut"
        Action {
            enabled: Boolean(d.verticalManipulator && d.verticalManipulator.zoomViewBy)
            onTriggered: d.handleWheel(0, -1, true, false)
        }
    }

    ActionItem {
        actionId: "org.diffscope.visualeditor.view.horizontalZoomIn"
        Action {
            enabled: Boolean(d.horizontalManipulator && d.horizontalManipulator.zoomViewBy)
            onTriggered: d.handleWheel(1, 0, true, false)
        }
    }

    ActionItem {
        actionId: "org.diffscope.visualeditor.view.horizontalZoomOut"
        Action {
            enabled: Boolean(d.horizontalManipulator && d.horizontalManipulator.zoomViewBy)
            onTriggered: d.handleWheel(-1, 0, true, false)
        }
    }

    ActionItem {
        actionId: "org.diffscope.visualeditor.view.resetVerticalScale"
        Action {
            enabled: Boolean(d.verticalManipulator && d.verticalManipulator.zoomViewBy)
            onTriggered: () => {
                if (d.addOn.activeEditingArea === 1) {
                    clavierManipulator.zoomViewBy(24 / d.pianoRollPanelInterface.clavierViewModel.pixelDensity, d.pianoRollPanelInterface.pianoRollView.centerEditArea.height / 2, true)
                }
            }
        }
    }

    ActionItem {
        actionId: "org.diffscope.visualeditor.view.resetHorizontalScale"
        Action {
            enabled: Boolean(d.horizontalManipulator && d.horizontalManipulator.zoomViewBy)
            onTriggered: () => {
                if (d.addOn.activeEditingArea === 0) {
                    arrangementPanelTimeManipulator.zoomViewBy(0.04 / d.arrangementPanelInterface.timeLayoutViewModel.pixelDensity, d.arrangementPanelInterface.arrangementView.centerEditArea.width / 2, true)
                } else if (d.addOn.activeEditingArea === 1) {
                    pianoRollPanelTimeManipulator.zoomViewBy(0.2 / d.pianoRollPanelInterface.timeLayoutViewModel.pixelDensity, d.pianoRollPanelInterface.pianoRollView.centerEditArea.width / 2, true)
                }
            }
        }
    }

}
