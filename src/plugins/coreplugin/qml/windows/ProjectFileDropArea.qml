// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

import QtQuick

import DiffScope.Core

DropArea {
    id: dropArea

    anchors.fill: parent
    z: -1

    function projectFilePaths(urls) {
        const filePaths = []
        for (const url of urls) {
            let fileUrl
            try {
                fileUrl = new URL(url.toString())
            } catch (error) {
                continue
            }
            if (fileUrl.protocol !== "file:")
                continue

            // The QML URL implementation returns a decoded pathname.
            let filePath = fileUrl.pathname
            if (!filePath.toLowerCase().endsWith(".dspx"))
                continue

            if (fileUrl.hostname) {
                filePath = "//" + fileUrl.hostname + filePath
            } else if (Qt.platform.os === "windows" && /^\/[A-Za-z]:\//.test(filePath)) {
                filePath = filePath.slice(1)
            }
            filePaths.push(filePath)
        }
        return filePaths
    }

    function acceptDrag(drag) {
        drag.accepted = false
        if (drag.hasUrls && (drag.supportedActions & Qt.CopyAction)
                && projectFilePaths(drag.urls).length > 0) {
            drag.accept(Qt.CopyAction)
        }
    }

    onEntered: drag => acceptDrag(drag)
    onPositionChanged: drag => acceptDrag(drag)

    onDropped: drop => {
        drop.accepted = false
        if (!drop.hasUrls || !(drop.supportedActions & Qt.CopyAction))
            return

        const filePaths = projectFilePaths(drop.urls)
        if (filePaths.length === 0)
            return

        const parentWindow = dropArea.Window.window
        drop.accept(Qt.CopyAction)
        Qt.callLater(() => {
            for (const filePath of filePaths)
                CoreInterface.openFile(filePath, parentWindow)
        })
    }
}
