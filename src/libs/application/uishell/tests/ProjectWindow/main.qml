// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

import QtQml
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQml.Models

import SVSCraft
import SVSCraft.UIComponents

import DiffScope.UIShell

ProjectWindow {
    id: window
    visible: true
    icon: "qrc:/qt/qml/DiffScope/UIShell/Test/ProjectWindow/test_icon.png"
    documentName: "test.dspx"

    component ListHelper: QtObject {
        default property list<QtObject> list: []
    }
    menusModel: ObjectModel {
        Menu {
            title: "&Test 1"

            Action {
                text: "aaa"
            }

            Action {
                text: "bbb"
            }
        }

        Menu {
            title: "T&est 2"

            Action {
                text: "aaa"
            }

            Action {
                text: "bbb"
            }
        }
    }
    leftToolButtonsModel: ObjectModel {
        ToolButton {
            icon.source: "qrc:/qt/qml/DiffScope/UIShell/assets/Grid16Filled.svg"
        }

        ToolButton {
            icon.source: "qrc:/qt/qml/DiffScope/UIShell/assets/Grid16Filled.svg"
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
        }

        ToolButton {
            icon.source: "qrc:/qt/qml/DiffScope/UIShell/assets/Grid16Filled.svg"
        }
    }
    statusButtonsModel: ObjectModel {
        Label {
            text: "status text"
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
        }

        ToolButton {
            icon.source: "qrc:/qt/qml/DiffScope/UIShell/assets/Grid16Filled.svg"
        }

        ToolButton {
            icon.source: "qrc:/qt/qml/DiffScope/UIShell/assets/Grid16Filled.svg"
        }
    }

    property DockingContext dockingContext: DockingContext {}

    property DockingHost dockingHost: DockingHost {
        context: window.dockingContext
        window: window
    }
    property list<DockingPane> panels: [
        DockingPane {
            panelKey: "first"
            title: "First Panel"
        },

        DockingPane {
            panelKey: "second"
            title: "Second Panel"
        },

        DockingPane {
            panelKey: "third"
            title: "Third Panel"
        },

        DockingPane {
            panelKey: "fourth"
            title: "Fourth Panel"
        }
    ]
    workspaceContent: DockingView {
        anchors.fill: parent
        host: window.dockingHost
    }
    Component.onCompleted: {
        for (const pane of panels)
            dockingContext.registerPanel(pane.panel)
        dockingHost.layout.restore({
            kind: "split",
            nodeId: "root",
            orientation: Qt.Horizontal,
            weights: [1, 3],
            children: [
                {
                    kind: "group",
                    nodeId: "side",
                    tabBarEdge: Qt.LeftEdge,
                    leading: {
                        panelIds: ["first", "second"],
                        selected: "first"
                    }
                },
                {
                    kind: "split",
                    nodeId: "center",
                    orientation: Qt.Vertical,
                    weights: [2, 3],
                    children: [
                        {
                            kind: "group",
                            nodeId: "top",
                            tabBarEdge: Qt.TopEdge,
                            leading: {
                                panelIds: ["third"],
                                selected: "third"
                            }
                        },
                        {
                            kind: "group",
                            nodeId: "bottom",
                            tabBarEdge: Qt.BottomEdge,
                            leading: {
                                panelIds: ["fourth"],
                                selected: "fourth"
                            }
                        }
                    ]
                }
            ]
        })
    }

    property Component bubbleNotificationHandle: QtObject {
        property string title: "111"
        property string text: "222"
        property int icon: SVS.NoIcon
        property list<string> buttons: []
        property int primaryButton: 0
        property bool closable: true
        property bool hasProgress: false
        property double progress: 0
        property bool progressAbortable: false
        property bool permanentlyHideable: true
        property int textFormat: Text.AutoText

        function hideClicked() {
            bnm.remove(ObjectModel.index)
        }

        function hoverEntered() {
            console.log("hover entered")
        }
    }

    bubbleNotificationsModel: ObjectModel {
        id: bnm

        QtObject {
            property string title: "Test"
            property string text: 'This is a test <a href="aaa">link</a>'
            property int icon: SVS.Information
            property list<string> buttons: ["AAA", "BBB"]
            property int primaryButton: 1
            property bool closable: true
            property bool hasProgress: true
            property double progress: -1
            property bool progressAbortable: true
            property bool permanentlyHideable: true

            function buttonClicked(index) {
                bnm.append(window.bubbleNotificationHandle.createObject())
            }

            function linkActivated(link) {
                console.log(link)
            }
            signal hoverEntered()
        }

        QtObject {
            property string title: "Error"
            property string text: ""
            property int icon: SVS.Critical
            property list<string> buttons: []
            property int primaryButton: 0
            property bool closable: true
            property bool hasProgress: false
            property double progress: 0
            property bool progressAbortable: false
            property bool permanentlyHideable: true
        }

        QtObject {
            property string title: "Warning"
            property string text: "aaa"
            property int icon: SVS.Warning
            property list<string> buttons: ["Yes", "No"]
            property int primaryButton: 0
            property bool closable: false
            property bool hasProgress: false
            property double progress: 0
            property bool progressAbortable: false
            property bool permanentlyHideable: false
        }
    }
}
