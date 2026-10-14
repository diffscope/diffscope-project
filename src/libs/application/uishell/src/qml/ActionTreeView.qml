// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

import QtQml.Models
import QtQuick
import QtQuick.Controls

import SVSCraft.UIComponents

TreeView {
    id: treeView

    signal indexSelected(var index)
    signal indexActivated(var index)
    signal contextMenuRequested(real x, real y)

    function selectIndex(index) {
        const alreadySelected = index.valid && selectionModel.isSelected(index)
        selectionModel.setCurrentIndex(index, ItemSelectionModel.ClearAndSelect)
        if (index.valid) {
            expandToIndex(index)
            if (alreadySelected)
                indexSelected(index)
        }
    }
    function activateIndex(index) {
        if (!index.valid)
            return false
        const row = rowAtIndex(index)
        if (row >= 0)
            toggleExpanded(row)
        indexActivated(index)
        return true
    }

    clip: true
    activeFocusOnTab: true
    acceptedButtons: Qt.NoButton
    boundsBehavior: Flickable.StopAtBounds
    boundsMovement: Flickable.StopAtBounds
    reuseItems: false
    selectionMode: TableView.SingleSelection
    selectionBehavior: TableView.SelectRows
    columnWidthProvider: column => width
    rowHeightProvider: row => 24
    onWidthChanged: forceLayout()

    selectionModel: ItemSelectionModel {
        model: treeView.model
        onCurrentChanged: (current, previous) => {
            if (current.valid)
                treeView.indexSelected(current)
        }
    }

    Keys.onReturnPressed: event => event.accepted = activateIndex(selectionModel.currentIndex)
    Keys.onEnterPressed: event => event.accepted = activateIndex(selectionModel.currentIndex)

    ScrollBar.vertical: ScrollBar {}
    ScrollBar.horizontal: ScrollBar {}
}
