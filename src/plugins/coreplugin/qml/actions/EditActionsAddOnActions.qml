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
        actionId: "org.diffscope.core.edit.undo"
        Action {
            onTriggered: {
                // TODO: Implement undo functionality
            }
        }
    }

    ActionItem {
        actionId: "org.diffscope.core.edit.redo"
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
        actionId: "org.diffscope.core.edit.cut"
        EditAction {
            flag: EditActionsHandler.Cut
        }
    }

    ActionItem {
        actionId: "org.diffscope.core.edit.copy"
        EditAction {
            flag: EditActionsHandler.Copy
        }
    }

    ActionItem {
        actionId: "org.diffscope.core.edit.paste"
        EditAction {
            flag: EditActionsHandler.Paste
        }
    }

    ActionItem {
        actionId: "org.diffscope.core.edit.pasteSpecial"
        EditAction {
            flag: EditActionsHandler.PasteSpecial
        }
    }

    ActionItem {
        actionId: "org.diffscope.core.edit.delete"
        EditAction {
            flag: EditActionsHandler.Delete
        }
    }

    ActionItem {
        actionId: "org.diffscope.core.edit.selectAll"
        EditAction {
            flag: EditActionsHandler.SelectAll
        }
    }

    ActionItem {
        actionId: "org.diffscope.core.edit.deselect"
        EditAction {
            flag: EditActionsHandler.Deselect
        }
    }

    ActionItem {
        actionId: "org.diffscope.core.edit.selectCurrent"
        EditAction {
            flag: EditActionsHandler.SelectCurrent
        }
    }

    // Selection navigation actions
    ActionItem {
        actionId: "org.diffscope.core.edit.selectUp"
        EditAction {
            flag: EditActionsHandler.SelectUp
        }
    }

    ActionItem {
        actionId: "org.diffscope.core.edit.selectDown"
        EditAction {
            flag: EditActionsHandler.SelectDown
        }
    }

    ActionItem {
        actionId: "org.diffscope.core.edit.selectLeft"
        EditAction {
            flag: EditActionsHandler.SelectLeft
        }
    }

    ActionItem {
        actionId: "org.diffscope.core.edit.selectRight"
        EditAction {
            flag: EditActionsHandler.SelectRight
        }
    }

    // Move actions
    ActionItem {
        actionId: "org.diffscope.core.edit.moveUp"
        MoveAction {
            direction: EditActionsHandler.ValueUp
        }
    }

    ActionItem {
        actionId: "org.diffscope.core.edit.moveDown"
        MoveAction {
            direction: EditActionsHandler.ValueDown
        }
    }

    ActionItem {
        actionId: "org.diffscope.core.edit.moveLeft"
        MoveAction {
            direction: EditActionsHandler.TimeBackward
        }
    }

    ActionItem {
        actionId: "org.diffscope.core.edit.moveRight"
        MoveAction {
            direction: EditActionsHandler.TimeForward
        }
    }

    ActionItem {
        actionId: "org.diffscope.core.edit.moveAction"
        Action {
            onTriggered: {
                // TODO: Implement move functionality
            }
        }
    }

    // Scroll actions
    ActionItem {
        actionId: "org.diffscope.core.view.scrollUp"
        EditAction {
            flag: EditActionsHandler.ScrollUp
        }
    }

    ActionItem {
        actionId: "org.diffscope.core.view.scrollDown"
        EditAction {
            flag: EditActionsHandler.ScrollDown
        }
    }

    ActionItem {
        actionId: "org.diffscope.core.view.scrollLeft"
        EditAction {
            flag: EditActionsHandler.ScrollLeft
        }
    }

    ActionItem {
        actionId: "org.diffscope.core.view.scrollRight"
        EditAction {
            flag: EditActionsHandler.ScrollRight
        }
    }

    // Cursor movement actions
    ActionItem {
        actionId: "org.diffscope.core.edit.moveCursorUp"
        EditAction {
            flag: EditActionsHandler.MoveCursorUp
        }
    }

    ActionItem {
        actionId: "org.diffscope.core.edit.moveCursorDown"
        EditAction {
            flag: EditActionsHandler.MoveCursorDown
        }
    }

    ActionItem {
        actionId: "org.diffscope.core.edit.moveCursorLeft"
        EditAction {
            flag: EditActionsHandler.MoveCursorLeft
        }
    }

    ActionItem {
        actionId: "org.diffscope.core.edit.moveCursorRight"
        EditAction {
            flag: EditActionsHandler.MoveCursorRight
        }
    }

    // Extend selection actions
    ActionItem {
        actionId: "org.diffscope.core.edit.extendSelectionUp"
        EditAction {
            flag: EditActionsHandler.ExtendSelectionUp
        }
    }

    ActionItem {
        actionId: "org.diffscope.core.edit.extendSelectionDown"
        EditAction {
            flag: EditActionsHandler.ExtendSelectionDown
        }
    }

    ActionItem {
        actionId: "org.diffscope.core.edit.extendSelectionLeft"
        EditAction {
            flag: EditActionsHandler.ExtendSelectionLeft
        }
    }

    ActionItem {
        actionId: "org.diffscope.core.edit.extendSelectionRight"
        EditAction {
            flag: EditActionsHandler.ExtendSelectionRight
        }
    }

    // Shrink selection actions
    ActionItem {
        actionId: "org.diffscope.core.edit.shrinkSelectionUp"
        EditAction {
            flag: EditActionsHandler.ShrinkSelectionUp
        }
    }

    ActionItem {
        actionId: "org.diffscope.core.edit.shrinkSelectionDown"
        EditAction {
            flag: EditActionsHandler.ShrinkSelectionDown
        }
    }

    ActionItem {
        actionId: "org.diffscope.core.edit.shrinkSelectionLeft"
        EditAction {
            flag: EditActionsHandler.ShrinkSelectionLeft
        }
    }

    ActionItem {
        actionId: "org.diffscope.core.edit.shrinkSelectionRight"
        EditAction {
            flag: EditActionsHandler.ShrinkSelectionRight
        }
    }

    // Page navigation actions
    ActionItem {
        actionId: "org.diffscope.core.view.pageUp"
        EditAction {
            flag: EditActionsHandler.PageUp
        }
    }

    ActionItem {
        actionId: "org.diffscope.core.view.pageDown"
        EditAction {
            flag: EditActionsHandler.PageDown
        }
    }

    ActionItem {
        actionId: "org.diffscope.core.view.pageLeft"
        EditAction {
            flag: EditActionsHandler.PageLeft
        }
    }

    ActionItem {
        actionId: "org.diffscope.core.view.pageRight"
        EditAction {
            flag: EditActionsHandler.PageRight
        }
    }

    // Scroll to position actions
    ActionItem {
        actionId: "org.diffscope.core.view.scrollToTop"
        EditAction {
            flag: EditActionsHandler.ScrollToValueTop
        }
    }

    ActionItem {
        actionId: "org.diffscope.core.view.scrollToBottom"
        EditAction {
            flag: EditActionsHandler.ScrollToValueBottom
        }
    }

    ActionItem {
        actionId: "org.diffscope.core.view.scrollToStart"
        EditAction {
            flag: EditActionsHandler.ScrollToTimeStart
        }
    }

    ActionItem {
        actionId: "org.diffscope.core.view.scrollToEnd"
        EditAction {
            flag: EditActionsHandler.ScrollToTimeEnd
        }
    }

    ActionItem {
        actionId: "org.diffscope.core.view.scrollToCurrentTime"
        EditAction {
            flag: EditActionsHandler.ScrollToCurrentTime
        }
    }

    ActionItem {
        actionId: "org.diffscope.core.timeline.goInsideViewRange"
        EditAction {
            flag: EditActionsHandler.GoInsideViewRange
        }
    }

}