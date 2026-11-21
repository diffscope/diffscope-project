// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

import QtQml
import QtQuick.Layouts

import dev.sjimo.ScopicFlow.Views

QtObject {
    readonly property Component absoluteTimelineTrackComponent: AbsoluteTimeline {
        required property QtObject contextObject

        readonly property ProjectViewModelContext projectViewModelContext: contextObject?.windowHandle?.ProjectViewModelContext.context ?? null

        Layout.fillWidth: true
        timeLayoutViewModel: contextObject?.timeLayoutViewModel ?? null
        timeViewModel: contextObject?.timeViewModel ?? null
        playbackViewModel: projectViewModelContext?.playbackViewModel ?? null
        timelineInteractionController: contextObject?.timelineInteractionController ?? null
    }
}
