// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

import QtQml
import QtQuick
import QtQuick.Controls

import QActionKit

import DiffScope.UIShell
import DiffScope.DspxModel as DspxModel
import DiffScope.DspxModel.SelectionModel as DspxSelectionModel

ActionCollection {
    id: d

    required property ProjectWindowInterface windowHandle
    readonly property QtObject transactionController: windowHandle?.projectDocumentContext.document?.transactionController ?? null

    ActionItem {
        actionId: "org.diffscope.core.edit.fillLyrics"
        Action {
            enabled: d.transactionController?.canBeginTransaction === true && d.transactionController.canAcquireReadLock
                     && d.windowHandle?.projectDocumentContext.document.anyItemsSelected
                     && d.windowHandle.projectDocumentContext.document.selectionModel.selectionType === DspxSelectionModel.SelectionModel.ST_Note
            onTriggered: Qt.callLater(() => d.windowHandle.fillLyricsScenario.fillLyrics(d.windowHandle.window))
        }
    }
}
