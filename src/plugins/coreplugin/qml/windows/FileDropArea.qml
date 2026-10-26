// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

import QtQuick

import DiffScope.Core

DropArea {
    id: dropArea

    anchors.fill: parent
    z: -1

    property FileOpenPlan pendingPlan: null
    property var pendingUrls: []

    function releasePlan() {
        const plan = pendingPlan
        pendingPlan = null
        pendingUrls = []
        if (plan)
            plan.discard()
    }

    function preparePlan(urls) {
        const urlStrings = []
        for (const url of urls)
            urlStrings.push(url.toString())

        const unchanged = pendingPlan && pendingPlan.valid
            && pendingUrls.length === urlStrings.length
            && pendingUrls.every((url, index) => url === urlStrings[index])
        if (unchanged)
            return

        releasePlan()
        pendingUrls = urlStrings
        pendingPlan = CoreInterface.fileOpenDispatcher.prepareDrop(urls, dropArea.Window.window)
    }

    function acceptDrag(drag) {
        drag.accepted = false
        if (!drag.hasUrls || !(drag.supportedActions & Qt.CopyAction)) {
            releasePlan()
            return
        }
        preparePlan(drag.urls)
        if (pendingPlan && pendingPlan.valid && pendingPlan.acceptsDrop)
            drag.accept(Qt.CopyAction)
    }

    onEntered: drag => acceptDrag(drag)
    onPositionChanged: drag => acceptDrag(drag)
    onExited: releasePlan()

    onDropped: drop => {
        drop.accepted = false
        if (!drop.hasUrls || !(drop.supportedActions & Qt.CopyAction)) {
            releasePlan()
            return
        }
        preparePlan(drop.urls)
        if (pendingPlan && pendingPlan.submit())
            drop.accept(Qt.CopyAction)
        releasePlan()
    }

    Component.onDestruction: releasePlan()
}
