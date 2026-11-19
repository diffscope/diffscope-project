// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

import QtQml
import QtQuick.Layouts

import dev.sjimo.ScopicFlow.Views

QtObject {
    readonly property Component absoluteTimelineTrackComponent: AbsoluteTimeline {
        required property QtObject contextObject

        Layout.fillWidth: true
        timeLayoutViewModel: contextObject?.timeLayoutViewModel ?? null
        timeViewModel: contextObject?.timeViewModel ?? null
    }
}
