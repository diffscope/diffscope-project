import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Templates as T
import QtQuick.Layouts

import QActionKit

import SVSCraft
import SVSCraft.UIComponents

import DiffScope.UIShell

QtObject {
    id: d
    required property QtObject addOn

    readonly property Component arrangementPanelComponent: ActionDockingPane {
        Component {
            id: dummyItem
            Item {}
        }
        header: ToolBarContainer {
            id: toolBar
            anchors.fill: parent
            property ActionInstantiator instantiator: ActionInstantiator {
                actionId: "org.diffscope.visualeditor.arrangementPanelToolBar"
                context: d.addOn?.windowHandle.actionContext ?? null
                separatorComponent: ToolBarContainerSeparator {
                }
                stretchComponent: ToolBarContainerStretch {
                }
                Component.onCompleted: forceUpdateLayouts()
                onObjectAdded: (index, object) => {
                    if (object instanceof Item) {
                        toolBar.insertItem(index, object)
                    } else if (object instanceof T.Action) {
                        toolBar.insertAction(index, object)
                    } else if (object instanceof T.Menu) {
                        toolBar.insertMenu(index, object)
                    } else {
                        toolBar.insertItem(index, dummyItem.createObject(this))
                    }
                }
                onObjectRemoved: (index, object) => {
                    if (object instanceof Item) {
                        toolBar.removeItem(object)
                    } else if (object instanceof Action) {
                        toolBar.removeAction(object)
                    } else if (object instanceof Menu) {
                        toolBar.removeMenu(object)
                    } else {
                        toolBar.removeItem(target.itemAt(index))
                    }
                }
            }
        }
        data: [d.addOn?.arrangementPanelInterface.arrangementView ?? null]
    }

}