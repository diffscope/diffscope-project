// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

pragma ComponentBehavior: Bound

import QtQml
import QtQuick
import QtQuick.Controls

Menu {
    id: menu

    required property DocumentProblems documentProblems
    property string problemId: ""
    property var actions: []

    function openFor(item: Item, position: point, targetId: string): void {
        const availableActions = documentProblems.actions(targetId)
        if (availableActions.length === 0)
            return
        problemId = targetId
        actions = availableActions
        popup(item, position)
    }

    Instantiator {
        model: menu.actions
        delegate: MenuItem {
            required property var modelData
            text: modelData.text
            onTriggered: menu.documentProblems.triggerAction(menu.problemId, modelData.id)
        }
        onObjectAdded: (index, item) => menu.insertItem(index, item)
        onObjectRemoved: (_, item) => menu.removeItem(item)
    }
}
