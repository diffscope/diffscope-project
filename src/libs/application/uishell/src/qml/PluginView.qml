import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Templates as T
import QtQuick.Controls.impl
import QtQuick.Layouts
import QtQml.Models

import SVSCraft
import SVSCraft.UIComponents
import SVSCraft.UIComponents.impl

Item {
    id: view
    property double navigationWidth: 320
    signal restartRequested()
    function askRestart() {
        if (MessageBox.question(qsTr("Restart %1?").replace("%1", Application.name), qsTr("After restart, plugin changes will be applied.")) === SVS.Yes) {
            restartRequested()
        }
    }
    component PluginCard: T.Button {
        id: pluginCard
        required property var modelData
        required property int index
        required property Repeater repeater
        Accessible.role: Accessible.ListItem
        Layout.fillWidth: true
        height: 60
        onClicked: () => {
            pluginDetailsPane.pluginSpec = modelData
        }
        Card {
            anchors.fill: parent
            atTop: pluginCard.index === 0
            atBottom: pluginCard.index === pluginCard.repeater.count - 1
            property color color: pluginCard.pressed ? Theme.controlPressedColorChange.apply(Theme.buttonColor) : pluginCard.hovered ? Theme.controlHoveredColorChange.apply(Theme.buttonColor) : Theme.backgroundTertiaryColor
            Behavior on color {
                ColorAnimation {
                    duration: Theme.colorAnimationDuration
                    easing.type: Easing.OutCubic
                }
            }
            onColorChanged: background.color = color
            title: RowLayout {
                spacing: 4
                Label {
                    text: pluginCard.modelData.displayName
                    font.weight: Font.DemiBold
                }
                Label {
                    text: pluginCard.modelData.version
                    ThemedItem.foregroundLevel: SVS.FL_Secondary
                }
            }
            subtitle: pluginCard.modelData.vendor
            image: Rectangle {
                color: Theme.backgroundQuaternaryColor
                ColorImage {
                    anchors.centerIn: parent
                    source: "qrc:/qt/qml/DiffScope/UIShell/assets/PuzzlePiece32Regular.svg"
                    color: Theme.foregroundSecondaryColor
                }
            }
            toolBar: RowLayout {
                ColorImage {
                    source: `qrc:/qt/qml/DiffScope/UIShell/assets/${pluginCard.modelData.hasError ? "DismissCircle16Filled" : !pluginCard.modelData.running ? "SubtractCircle16Filled" : pluginCard.modelData.required ? "CheckmarkLock16Filled" : "CheckmarkCircle16Filled"}.svg`
                    color: pluginCard.modelData.hasError ? Theme.errorColor : !pluginCard.modelData.running ? Theme.foregroundSecondaryColor : Theme.accentColor
                }
                Switch {
                    enabled: !pluginCard.modelData.required
                    checked: pluginCard.modelData.enabledBySettings
                    onCheckedChanged: pluginCard.modelData.enabledBySettings = checked
                }
            }
        }
        T.Button {
            id: restartButton
            visible: pluginCard.modelData.restartRequired
            width: implicitContentWidth + 4
            height: 14
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.margins: 4
            text: qsTr("Restart required")
            background: Rectangle {
                color: restartButton.pressed ? Theme.controlPressedColorChange.apply(Theme.accentColor) :
                       restartButton.hovered ? Theme.controlHoveredColorChange.apply(Theme.accentColor) :
                       Theme.accentColor
            }
            contentItem: Text {
                color: Theme.foregroundPrimaryColor
                font.pixelSize: 11
                horizontalAlignment: Text.AlignHCenter
                text: restartButton.text
            }
            onClicked: () => {
                view.askRestart()
            }
        }

    }
    QtObject {
        id: pluginManagerHelper
        property var pluginCollections: [
            {
                name: "Test 1",
                plugins: [
                    {
                        name: "Plugin 1",
                        displayName: "Plugin 1",
                        version: "0.0.1.0",
                        compatVersion: "0.0.0.0",
                        vendor: "Vendor 1",
                        copyright: "Copyright",
                        license: "License",
                        description: "Description",
                        url: "https://example.com/",
                        category: "Test 1",
                        availableForHostPlatform: true,
                        required: true,
                        enabledIndirectly: true,
                        forceEnabled: true,
                        forceDisabled: true,
                        hasError: false,
                        errorString: "",
                        enabledBySettings: true,
                        restartRequired: true,
                        running: true,
                    },
                    {
                        name: "Plugin 2",
                        displayName: "Plugin 2",
                        version: "0.0.1.0",
                        compatVersion: "0.0.0.0",
                        vendor: "Vendor 1",
                        copyright: "Copyright",
                        license: "License",
                        description: "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua.",
                        url: "URL",
                        category: "Test 1",
                        availableForHostPlatform: true,
                        required: false,
                        enabledIndirectly: true,
                        forceEnabled: true,
                        forceDisabled: true,
                        hasError: true,
                        errorString: "",
                        enabledBySettings: true,
                        restartRequired: false,
                        running: false,
                    },
                ],
            },
            {
                name: "Test 2",
                plugins: [],
            },
            {
                name: "Test 3",
                plugins: [],
            },
        ]
    }
    SplitView {
        anchors.fill: parent
        Rectangle {
            SplitView.fillHeight: true
            SplitView.preferredWidth: view.navigationWidth
            onWidthChanged: GlobalHelper.setProperty(view, "navigationWidth", width)
            color: Theme.backgroundTertiaryColor
            ColumnLayout {
                anchors.fill: parent
                TextField {
                    Layout.fillWidth: true
                    Layout.margins: 8
                    placeholderText: qsTr("Search")
                    ThemedItem.icon.source: "qrc:/qt/qml/DiffScope/UIShell/assets/Search16Filled.svg"
                }
                ScrollView {
                    id: pluginsScrollView
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    ColumnLayout {
                        width: pluginsScrollView.width
                        spacing: 4
                        Repeater {
                            model: pluginManagerHelper.pluginCollections
                            delegate: GroupBox {
                                id: pluginCollectionGroupBox
                                required property var modelData
                                Layout.fillWidth: true
                                title: modelData.name
                                leftPadding: 8
                                rightPadding: 8
                                ColumnLayout {
                                    width: pluginCollectionGroupBox.width - pluginCollectionGroupBox.leftPadding - pluginCollectionGroupBox.rightPadding
                                    spacing: 0
                                    Repeater {
                                        id: pluginCollectionRepeater
                                        model: pluginCollectionGroupBox.modelData.plugins
                                        delegate: PluginCard {
                                            repeater: pluginCollectionRepeater
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        Pane {
            id: pluginDetailsPane
            property var pluginSpec: null
            SplitView.fillHeight: true
            padding: 16
            ColumnLayout {
                id: pluginDetailsLayout
                visible: pluginDetailsPane.pluginSpec !== null
                spacing: 16
                anchors.fill: parent
                Label {
                    text: pluginDetailsPane.pluginSpec?.displayName ?? ""
                    font.pixelSize: 24
                    font.weight: Font.Medium
                    DescriptiveText.toolTip: qsTr("Plugin name")
                    DescriptiveText.activated: pluginNameHoverHandler.hovered
                    HoverHandler {
                        id: pluginNameHoverHandler
                    }
                }
                RowLayout {
                    spacing: 16
                    Layout.maximumHeight: implicitHeight
                    Label {
                        text: pluginDetailsPane.pluginSpec?.vendor ?? ""
                        DescriptiveText.toolTip: qsTr("Vendor")
                        DescriptiveText.activated: vendorHoverHandler.hovered
                        HoverHandler {
                            id: vendorHoverHandler
                        }
                    }
                    Rectangle {
                        Layout.fillHeight: true
                        width: 1
                        color: Theme.foregroundSecondaryColor
                    }
                    Label {
                        text: pluginDetailsPane.pluginSpec?.version ?? ""
                        DescriptiveText.toolTip: qsTr("Version")
                        DescriptiveText.activated: versionHoverHandler.hovered
                        HoverHandler {
                            id: versionHoverHandler
                        }
                    }
                    Rectangle {
                        Layout.fillHeight: true
                        width: 1
                        color: Theme.foregroundSecondaryColor
                        visible: (pluginDetailsPane.pluginSpec?.url ?? "").length !== 0
                    }
                    RowLayout {
                        spacing: 0
                        visible: (pluginDetailsPane.pluginSpec?.url ?? "").length !== 0
                        Label {
                            text: `<a href="${pluginDetailsPane.pluginSpec?.url ?? ""}" style="text-decoration: ${hoveredLink ? "underline" : "none"}; color: rgba(${Theme.linkColor.r * 255}, ${Theme.linkColor.g * 255}, ${Theme.linkColor.b * 255}, ${Theme.linkColor.a});">${qsTr("Visit plugin homepage")}</a>` // TODO toHtmlEscaped
                            textFormat: Text.RichText
                            onLinkActivated: (link) => Qt.openUrlExternally(link)
                        }
                        ColorImage {
                            source: "qrc:/qt/qml/DiffScope/UIShell/assets/ArrowUpRight12Filled.svg"
                            color: Theme.linkColor
                        }
                    }
                }
                Label {
                    text: pluginDetailsPane.pluginSpec?.description ?? ""
                    Layout.maximumWidth: pluginDetailsLayout.width
                    DescriptiveText.toolTip: qsTr("Description")
                    DescriptiveText.activated: descriptionHoverHandler.hovered
                    wrapMode: Text.Wrap
                    HoverHandler {
                        id: descriptionHoverHandler
                    }
                }
                RowLayout {
                    spacing: 8
                    Layout.maximumHeight: implicitHeight
                    Switch {
                        enabled: !(pluginDetailsPane.pluginSpec?.required ?? false)
                        checked: pluginDetailsPane.pluginSpec?.enabledBySettings ?? false
                        onCheckedChanged: if (pluginDetailsPane.pluginSpec) pluginDetailsPane.pluginSpec.enabledBySettings = checked
                        text: !enabled ? qsTr("Required") : checked ? qsTr("Enabled") : qsTr("Disabled")
                        padding: 0
                    }
                    Button {
                        implicitHeight: 24
                        visible: pluginDetailsPane.pluginSpec?.restartRequired ?? false
                        text: qsTr("Restart required")
                        ThemedItem.controlType: SVS.CT_Accent
                        onClicked: view.askRestart()
                    }
                }
                Rectangle {
                    Layout.fillWidth: true
                    height: 1
                    color: Theme.foregroundSecondaryColor
                }
                Label {
                    Layout.maximumWidth: pluginDetailsLayout.width
                    wrapMode: Text.Wrap
                    text: qsTr("Category: %1").replace("%1", pluginDetailsPane.pluginSpec?.category ?? "")
                }
                Label {
                    Layout.maximumWidth: pluginDetailsLayout.width
                    wrapMode: Text.Wrap
                    text: qsTr("Compat version: %1").replace("%1", pluginDetailsPane.pluginSpec?.compatVersion ?? "")
                }
                Label {
                    Layout.maximumWidth: pluginDetailsLayout.width
                    wrapMode: Text.Wrap
                    visible: (pluginDetailsPane.pluginSpec?.copyright ?? "").length !== 0
                    text: qsTr("Copyright: %1").replace("%1", pluginDetailsPane.pluginSpec?.copyright ?? "")
                }
                Label {
                    Layout.maximumWidth: pluginDetailsLayout.width
                    wrapMode: Text.Wrap
                    visible: (pluginDetailsPane.pluginSpec?.license ?? "").length !== 0
                    text: qsTr("License: %1").replace("%1", pluginDetailsPane.pluginSpec?.license ?? "")
                }
                Item {
                    Layout.fillHeight: true
                }
            }
        }
    }
}