import QtQml
import QtQuick
import QtQuick.Controls

import QActionKit

ActionCollection {
    id: d

    required property QtObject addOn
    readonly property ProjectWindowInterface windowHandle: addOn?.windowHandle ?? null

    // Basic edit actions
    ActionItem {
        actionId: "core.edit.undo"
        Action {
            onTriggered: {
                // TODO: Implement undo functionality
            }
        }
    }

    ActionItem {
        actionId: "core.edit.redo"
        Action {
            onTriggered: {
                // TODO: Implement redo functionality
            }
        }
    }

    component EditAction: Action {
        required property int flag
        enabled: d.windowHandle.mainEditActionsHandlerRegistry.enabledActions & flag
        onTriggered: d.windowHandle.mainEditActionsHandlerRegistry.triggerEditAction(flag)
    }

    component MoveAction: Action {
        required property int direction
        enabled: d.windowHandle.mainEditActionsHandlerRegistry.enabledMoveDirections & direction
        onTriggered: d.windowHandle.mainEditActionsHandlerRegistry.move(direction, 0)
    }

    ActionItem {
        actionId: "core.edit.cut"
        EditAction {
            flag: EditActionsHandler.Cut
        }
    }

    ActionItem {
        actionId: "core.edit.copy"
        EditAction {
            flag: EditActionsHandler.Copy
        }
    }

    ActionItem {
        actionId: "core.edit.paste"
        EditAction {
            flag: EditActionsHandler.Paste
        }
    }

    ActionItem {
        actionId: "core.edit.pasteSpecial"
        EditAction {
            flag: EditActionsHandler.PasteSpecial
        }
    }

    ActionItem {
        actionId: "core.edit.delete"
        EditAction {
            flag: EditActionsHandler.Delete
        }
    }

    ActionItem {
        actionId: "core.edit.selectAll"
        EditAction {
            flag: EditActionsHandler.SelectAll
        }
    }

    ActionItem {
        actionId: "core.edit.deselect"
        EditAction {
            flag: EditActionsHandler.Deselect
        }
    }

    ActionItem {
        actionId: "core.edit.selectCurrent"
        EditAction {
            flag: EditActionsHandler.SelectCurrent
        }
    }

    // Selection navigation actions
    ActionItem {
        actionId: "core.edit.selectUp"
        EditAction {
            flag: EditActionsHandler.SelectUp
        }
    }

    ActionItem {
        actionId: "core.edit.selectDown"
        EditAction {
            flag: EditActionsHandler.SelectDown
        }
    }

    ActionItem {
        actionId: "core.edit.selectLeft"
        EditAction {
            flag: EditActionsHandler.SelectLeft
        }
    }

    ActionItem {
        actionId: "core.edit.selectRight"
        EditAction {
            flag: EditActionsHandler.SelectRight
        }
    }

    // Move actions
    ActionItem {
        actionId: "core.edit.moveUp"
        MoveAction {
            direction: EditActionsHandler.ValueUp
        }
    }

    ActionItem {
        actionId: "core.edit.moveDown"
        MoveAction {
            direction: EditActionsHandler.ValueDown
        }
    }

    ActionItem {
        actionId: "core.edit.moveLeft"
        MoveAction {
            direction: EditActionsHandler.TimeBackward
        }
    }

    ActionItem {
        actionId: "core.edit.moveRight"
        MoveAction {
            direction: EditActionsHandler.TimeForward
        }
    }

    ActionItem {
        actionId: "core.edit.moveAction"
        Action {
            onTriggered: {
                // TODO: Implement move functionality
            }
        }
    }

    // Scroll actions
    ActionItem {
        actionId: "core.view.scrollUp"
        EditAction {
            flag: EditActionsHandler.ScrollUp
        }
    }

    ActionItem {
        actionId: "core.view.scrollDown"
        EditAction {
            flag: EditActionsHandler.ScrollDown
        }
    }

    ActionItem {
        actionId: "core.view.scrollLeft"
        EditAction {
            flag: EditActionsHandler.ScrollLeft
        }
    }

    ActionItem {
        actionId: "core.view.scrollRight"
        EditAction {
            flag: EditActionsHandler.ScrollRight
        }
    }

    // Cursor movement actions
    ActionItem {
        actionId: "core.edit.moveCursorUp"
        EditAction {
            flag: EditActionsHandler.MoveCursorUp
        }
    }

    ActionItem {
        actionId: "core.edit.moveCursorDown"
        EditAction {
            flag: EditActionsHandler.MoveCursorDown
        }
    }

    ActionItem {
        actionId: "core.edit.moveCursorLeft"
        EditAction {
            flag: EditActionsHandler.MoveCursorLeft
        }
    }

    ActionItem {
        actionId: "core.edit.moveCursorRight"
        EditAction {
            flag: EditActionsHandler.MoveCursorRight
        }
    }

    // Extend selection actions
    ActionItem {
        actionId: "core.edit.extendSelectionUp"
        EditAction {
            flag: EditActionsHandler.ExtendSelectionUp
        }
    }

    ActionItem {
        actionId: "core.edit.extendSelectionDown"
        EditAction {
            flag: EditActionsHandler.ExtendSelectionDown
        }
    }

    ActionItem {
        actionId: "core.edit.extendSelectionLeft"
        EditAction {
            flag: EditActionsHandler.ExtendSelectionLeft
        }
    }

    ActionItem {
        actionId: "core.edit.extendSelectionRight"
        EditAction {
            flag: EditActionsHandler.ExtendSelectionRight
        }
    }

    // Shrink selection actions
    ActionItem {
        actionId: "core.edit.shrinkSelectionUp"
        EditAction {
            flag: EditActionsHandler.ShrinkSelectionUp
        }
    }

    ActionItem {
        actionId: "core.edit.shrinkSelectionDown"
        EditAction {
            flag: EditActionsHandler.ShrinkSelectionDown
        }
    }

    ActionItem {
        actionId: "core.edit.shrinkSelectionLeft"
        EditAction {
            flag: EditActionsHandler.ShrinkSelectionLeft
        }
    }

    ActionItem {
        actionId: "core.edit.shrinkSelectionRight"
        EditAction {
            flag: EditActionsHandler.ShrinkSelectionRight
        }
    }

    // Page navigation actions
    ActionItem {
        actionId: "core.view.pageUp"
        EditAction {
            flag: EditActionsHandler.PageUp
        }
    }

    ActionItem {
        actionId: "core.view.pageDown"
        EditAction {
            flag: EditActionsHandler.PageDown
        }
    }

    ActionItem {
        actionId: "core.view.pageLeft"
        EditAction {
            flag: EditActionsHandler.PageLeft
        }
    }

    ActionItem {
        actionId: "core.view.pageRight"
        EditAction {
            flag: EditActionsHandler.PageRight
        }
    }

    // Scroll to position actions
    ActionItem {
        actionId: "core.view.scrollToTop"
        EditAction {
            flag: EditActionsHandler.ScrollToValueTop
        }
    }

    ActionItem {
        actionId: "core.view.scrollToBottom"
        EditAction {
            flag: EditActionsHandler.ScrollToValueBottom
        }
    }

    ActionItem {
        actionId: "core.view.scrollToStart"
        EditAction {
            flag: EditActionsHandler.ScrollToTimeStart
        }
    }

    ActionItem {
        actionId: "core.view.scrollToEnd"
        EditAction {
            flag: EditActionsHandler.ScrollToTimeEnd
        }
    }

    ActionItem {
        actionId: "core.view.scrollToCurrentTime"
        EditAction {
            flag: EditActionsHandler.ScrollToCurrentTime
        }
    }

    ActionItem {
        actionId: "core.timeline.goInsideViewRange"
        EditAction {
            flag: EditActionsHandler.GoInsideViewRange
        }
    }

}