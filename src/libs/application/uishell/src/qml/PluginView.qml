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
    property var provider: PluginManagerHelper {

    }
    property string currentPluginName: ""
    property bool useSplitView: true
    clip: true

    onCurrentPluginNameChanged: () => {
        if (currentPluginName === d.pluginSpec?.name) {
            return
        }
        if (currentPluginName === "") {
            d.pluginSpec = null
            return
        }
        d.pluginSpec = provider.findPlugin(currentPluginName)
    }

    signal restartRequested()
    function askRestart() {
        if (MessageBox.question(qsTr("Restart %1?").replace("%1", Application.name), qsTr("After restart, plugin changes will be applied.")) === SVS.Yes) {
            restartRequested()
        }
    }
    function handlePluginToggle(pluginSpec, button) {
        if (button.checked) {
            let deps = pluginSpec.dependencies.filter(p => !p.enabledBySettings)
            if (deps.length === 0) {
                pluginSpec.enabledBySettings = true
                return
            }
            if (MessageBox.question(
                qsTr("Enabling %1 will also enable the following plugins:").replace("%1", pluginSpec.displayName),
                deps.map(p => p.displayName).join("\n") + "\n\n" + qsTr("Continue?")
            ) === SVS.No) {
                button.checked = false
                return
            }
            pluginSpec.enabledBySettings = true
            for (let p of deps) {
                p.enabledBySettings = true
            }
        } else {
            let deps = pluginSpec.dependents.filter(p => p.enabledBySettings)
            if (deps.length === 0) {
                pluginSpec.enabledBySettings = false
                return
            }
            if (MessageBox.question(
                qsTr("Disabling %1 will also disable the following plugins:").replace("%1", pluginSpec.displayName),
                deps.map(p => p.displayName).join("\n") + "\n\n" + qsTr("Continue?")
            ) === SVS.No) {
                button.checked = true
                return
            }
            pluginSpec.enabledBySettings = false
            for (let p of deps) {
                p.enabledBySettings = false
            }
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
            d.pluginSpec = modelData
        }
        Card {
            anchors.fill: parent
            atTop: pluginCard.index === 0
            atBottom: pluginCard.index === pluginCard.repeater.count - 1
            property color _baseColor: view.useSplitView ? Theme.backgroundTertiaryColor : Theme.backgroundColor(view.ThemedItem.backgroundLevel)
            property color color: pluginCard.pressed ? Theme.controlPressedColorChange.apply(_baseColor) : pluginCard.hovered ? Theme.controlHoveredColorChange.apply(_baseColor) : _baseColor
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
                IconLabel {
                    anchors.centerIn: parent
                    icon.source: "qrc:/qt/qml/DiffScope/UIShell/assets/PuzzlePiece32Regular.svg"
                    icon.color: Theme.foregroundSecondaryColor
                }
            }
            toolBar: RowLayout {
                IconLabel {
                    icon.source: `qrc:/qt/qml/DiffScope/UIShell/assets/${pluginCard.modelData.hasError ? "DismissCircle16Filled" : !pluginCard.modelData.running ? "SubtractCircle16Filled" : pluginCard.modelData.required ? "CheckmarkLock16Filled" : "CheckmarkCircle16Filled"}.svg`
                    icon.color: pluginCard.modelData.hasError ? Theme.errorColor : !pluginCard.modelData.running ? Theme.foregroundSecondaryColor : Theme.accentColor
                }
                Switch {
                    enabled: !pluginCard.modelData.required
                    checked: pluginCard.modelData.enabledBySettings
                    onClicked: view.handlePluginToggle(pluginCard.modelData, this)
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
    component SelectableLabel: TextEdit {
        readOnly: true
        color: Theme.foregroundColor(ThemedItem.foregroundLevel)
        Accessible.role: Accessible.StaticText
        Accessible.name: text
        selectionColor: Theme.accentColor
    }
    component InfoCard: Frame {
        id: card
        Layout.fillWidth: true
        padding: 8
        property string title: ""
        property string text: ""
        background: Rectangle {
            color: Theme.backgroundPrimaryColor
            border.width: 1
            border.color: Theme.borderColor
            radius: 4
        }
        ColumnLayout {
            spacing: 4
            anchors.fill: parent
            Label {
                text: card.title
                font.pixelSize: 14
                font.weight: Font.DemiBold
            }
            SelectableLabel {
                Layout.fillWidth: true
                ThemedItem.foregroundLevel: SVS.FL_Secondary
                text: card.text
                wrapMode: Text.Wrap
            }
        }
    }
    component PluginDetailsPaneContent: ColumnLayout {
        visible: d.pluginSpec !== null
        spacing: 16
        anchors.fill: parent
        ColumnLayout {
            Layout.fillWidth: true
            Layout.leftMargin: 16
            Layout.rightMargin: 16
            spacing: 16
            RowLayout {
                spacing: 8
                Layout.maximumHeight: implicitHeight
                Button {
                    visible: !view.useSplitView
                    icon.source: "qrc:/qt/qml/DiffScope/UIShell/assets/ArrowLeft24Filled.svg"
                    text: qsTr("Back")
                    flat: true
                    display: AbstractButton.IconOnly
                    DescriptiveText.toolTip: text
                    DescriptiveText.activated: hovered
                    onClicked: pluginDetailsPane.StackView.view.pop()
                }
                SelectableLabel {
                    text: d.pluginSpec.displayName
                    font.pixelSize: 24
                    font.weight: Font.Medium

                    DescriptiveText.toolTip: qsTr("Plugin name")
                    DescriptiveText.activated: pluginNameHoverHandler.hovered
                    HoverHandler {
                        id: pluginNameHoverHandler
                    }
                }
            }
            RowLayout {
                spacing: 16
                Layout.maximumHeight: implicitHeight
                SelectableLabel {
                    text: d.pluginSpec.vendor
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
                SelectableLabel {
                    text: d.pluginSpec.version
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
                    visible: (d.pluginSpec.url).length !== 0
                }
                LinkLabel {
                    visible: href.length !== 0
                    href: d.pluginSpec.url
                    linkText: qsTr("Visit plugin homepage")
                    externalLink: true
                    onLinkActivated: (link) => Qt.openUrlExternally(link)
                }
            }
            SelectableLabel {
                text: d.pluginSpec.description
                Layout.maximumWidth: pluginDetailsPane.width - 32
                DescriptiveText.toolTip: qsTr("Description")
                DescriptiveText.activated: descriptionHoverHandler.hovered
                wrapMode: Text.Wrap
                HoverHandler {
                    id: descriptionHoverHandler
                }
            }
            RowLayout {
                spacing: 16
                Layout.preferredHeight: 24
                Switch {
                    enabled: !(d.pluginSpec.required)
                    checked: d.pluginSpec.enabledBySettings
                    text: !enabled ? qsTr("Required") : checked ? qsTr("Enabled") : qsTr("Disabled")
                    padding: 0
                    onClicked: view.handlePluginToggle(d.pluginSpec, this)
                }
                IconLabel {
                    spacing: 4
                    icon.source: `qrc:/qt/qml/DiffScope/UIShell/assets/${d.pluginSpec?.hasError ? "DismissCircle16Filled" : !d.pluginSpec?.running ? "SubtractCircle16Filled" : d.pluginSpec?.required ? "CheckmarkLock16Filled" : "CheckmarkCircle16Filled"}.svg`
                    icon.color: d.pluginSpec?.hasError ? Theme.errorColor : !d.pluginSpec?.running ? Theme.foregroundSecondaryColor : Theme.accentColor
                    text: d.pluginSpec?.hasError ? qsTr("Plugin status: Error") : !d.pluginSpec?.running ? qsTr("Plugin status: Not loaded") : qsTr("Plugin status: Loaded")
                    color: Theme.foregroundPrimaryColor
                }
                Button {
                    implicitHeight: 24
                    visible: d.pluginSpec.restartRequired
                    text: qsTr("Restart required")
                    ThemedItem.controlType: SVS.CT_Accent
                    onClicked: view.askRestart()
                }
            }
            InfoCard {
                visible: d.pluginSpec.hasError
                title: qsTr("Error details")
                text: d.pluginSpec.errorString
            }
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 0
                TabBar {
                    id: tabBar
                    ThemedItem.flat: true
                    TabButton {
                        text: qsTr("Details")
                        ThemedItem.tabIndicator: SVS.TI_Bottom
                        width: implicitWidth
                    }
                    TabButton {
                        text: qsTr("Dependencies")
                        ThemedItem.tabIndicator: SVS.TI_Bottom
                        width: implicitWidth
                        DescriptiveText.toolTip: qsTr("Plugins required by this plugin, including indirect dependencies")
                        DescriptiveText.activated: hovered
                    }
                    TabButton {
                        text: qsTr("Dependents")
                        ThemedItem.tabIndicator: SVS.TI_Bottom
                        width: implicitWidth
                        DescriptiveText.toolTip: qsTr("Plugins that require this plugin, including indirect dependants")
                        DescriptiveText.activated: hovered
                    }
                    TabButton {
                        text: qsTr("Additional Info")
                        ThemedItem.tabIndicator: SVS.TI_Bottom
                        width: implicitWidth
                        enabled: additionalInfoLabel.text.length !== 0
                    }
                }
                Rectangle {
                    Layout.fillWidth: true
                    height: 1
                    color: Theme.borderColor
                }
            }
        }
        StackLayout {
            currentIndex: tabBar.currentIndex
            Layout.fillWidth: true
            Layout.fillHeight: true
            ScrollView {
                id: detailsScrollView
                ColumnLayout {
                    width: detailsScrollView.width
                    ColumnLayout {
                        Layout.fillWidth: true
                        Layout.leftMargin: 16
                        Layout.rightMargin: 16
                        Layout.bottomMargin: 16
                        spacing: 16
                        InfoCard {
                            title: qsTr("Identifier")
                            text: d.pluginSpec.name
                        }
                        InfoCard {
                            title: qsTr("Category")
                            text: d.pluginSpec.category
                        }
                        Frame {
                            Layout.fillWidth: true
                            padding: 8
                            background: Rectangle {
                                color: Theme.backgroundPrimaryColor
                                border.width: 1
                                border.color: Theme.borderColor
                                radius: 4
                            }
                            RowLayout {
                                spacing: 4
                                anchors.fill: parent
                                ColumnLayout {
                                    spacing: 4
                                    Layout.fillWidth: true
                                    Label {
                                        text: qsTr("Path")
                                        font.pixelSize: 14
                                        font.weight: Font.DemiBold
                                    }
                                    SelectableLabel {
                                        Layout.fillWidth: true
                                        ThemedItem.foregroundLevel: SVS.FL_Secondary
                                        text: d.pluginSpec.filePath
                                        wrapMode: Text.Wrap
                                    }
                                }
                                ToolButton {
                                    icon.source: "qrc:/qt/qml/DiffScope/UIShell/assets/Open16Filled"
                                    display: AbstractButton.IconOnly
                                    text: qsTr("Reveal in %1").replace("%1", Qt.platform.os === "osx" || Qt.platform.os === "macos" ? qsTr("Finder") : qsTr("File Explorer"))
                                    DescriptiveText.toolTip: text
                                    DescriptiveText.activated: hovered
                                    onClicked: () => {
                                        DesktopServices.reveal(d.pluginSpec.filePath)
                                    }
                                }
                            }

                        }
                        InfoCard {
                            title: qsTr("Compat version")
                            text: d.pluginSpec.compatVersion
                        }
                        InfoCard {
                            visible: text.length !== 0
                            title: qsTr("Copyright")
                            text: d.pluginSpec.copyright
                        }
                        InfoCard {
                            visible: text.length !== 0
                            title: qsTr("License")
                            text: d.pluginSpec.license
                        }
                    }
                }
            }
            ScrollView {
                id: dependencyScrollView
                ColumnLayout {
                    width: dependencyScrollView.width
                    ColumnLayout {
                        Layout.fillWidth: true
                        Layout.leftMargin: 16
                        Layout.rightMargin: 16
                        Layout.bottomMargin: 16
                        spacing: 4
                        Label {
                            Layout.fillWidth: true
                            wrapMode: Text.Wrap
                            text: qsTr("This plugin does not require other plugins.")
                            visible: dependencyRepeater.count === 0
                        }
                        Repeater {
                            id: dependencyRepeater
                            model: d.pluginSpec.dependencies
                            delegate: LinkLabel {
                                required property var modelData
                                Layout.fillWidth: true
                                wrapMode: Text.Wrap
                                href: modelData.name
                                linkText: modelData.displayName
                                onLinkActivated: d.pluginSpec = modelData
                            }
                        }
                    }
                }
            }
            ScrollView {
                id: dependentScrollView
                ColumnLayout {
                    width: dependentScrollView.width
                    ColumnLayout {
                        Layout.fillWidth: true
                        Layout.leftMargin: 16
                        Layout.rightMargin: 16
                        Layout.bottomMargin: 16
                        spacing: 4
                        Label {
                            Layout.fillWidth: true
                            wrapMode: Text.Wrap
                            text: qsTr("No plugins require this plugin.")
                            visible: dependentRepeater.count === 0
                        }
                        Repeater {
                            id: dependentRepeater
                            model: d.pluginSpec.dependents
                            delegate: LinkLabel {
                                required property var modelData
                                Layout.fillWidth: true
                                wrapMode: Text.Wrap
                                href: modelData.name
                                linkText: modelData.displayName
                                onLinkActivated: d.pluginSpec = modelData
                            }
                        }
                    }
                }
            }
            ColumnLayout {
                SelectableLabel {
                    id: additionalInfoLabel
                    Layout.fillWidth: true
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16
                    Layout.bottomMargin: 16
                    wrapMode: Text.Wrap
                    text: {
                        let spec = d.pluginSpec
                        if (!spec)
                            return ""
                        if (!spec.availableForHostPlatform)
                            return qsTr("Plugin is not available on this platform.");
                        if (spec.enabledIndirectly)
                            return qsTr("Plugin is enabled as dependency of an enabled plugin.");
                        if (spec.forceEnabled)
                            return qsTr("Plugin is enabled by command line argument.");
                        if (spec.forceDisabled)
                            return qsTr("Plugin is disabled by command line argument.");
                        return ""
                    }
                }
            }
        }
    }
    component PluginListView : Rectangle {
        color: view.useSplitView ? Theme.backgroundTertiaryColor : Theme.backgroundColor(view.ThemedItem.backgroundLevel)
        ColumnLayout {
            anchors.fill: parent
            spacing: 8
            RowLayout {
                spacing: 8
                Layout.margins: 8
                Layout.fillWidth: true
                TextField {
                    id: searchTextField
                    Layout.fillWidth: true
                    placeholderText: qsTr("Search")
                    ThemedItem.icon.source: "qrc:/qt/qml/DiffScope/UIShell/assets/Search16Filled.svg"
                }
            }

            ScrollView {
                id: pluginsScrollView
                Layout.fillWidth: true
                Layout.fillHeight: true
                ColumnLayout {
                    width: pluginsScrollView.width
                    spacing: 0
                    Label {
                        text: qsTr("No result found")
                        ThemedItem.foregroundLevel: SVS.FL_Secondary
                        Layout.alignment: Qt.AlignHCenter
                        visible: pluginCollectionLayout.visibleChildren.length === 1
                    }
                    ColumnLayout {
                        id: pluginCollectionLayout
                        spacing: 8
                        Repeater {
                            model: view.provider.pluginCollections
                            delegate: GroupBox {
                                id: pluginCollectionGroupBox
                                required property var modelData
                                Layout.fillWidth: true
                                title: modelData.name
                                leftPadding: 8
                                rightPadding: 8
                                visible: (searchTextField.text.length === 0 && title.length !== 0) || pluginCollectionRepeater.count !== 0
                                ColumnLayout {
                                    width: pluginCollectionGroupBox.width - pluginCollectionGroupBox.leftPadding - pluginCollectionGroupBox.rightPadding
                                    spacing: 0
                                    Repeater {
                                        id: pluginCollectionRepeater
                                        model: pluginCollectionGroupBox.modelData.plugins.filter(p => searchTextField.text.length === 0 || [p.name, p.displayName, p.description].some(s => s.toLowerCase().indexOf(searchTextField.text.toLowerCase()) !== -1))
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
    }
    component PluginDetailsView: Pane {
        id: pluginDetailsPane
        readonly property var pluginSpec: d.pluginSpec
        ThemedItem.backgroundLevel: view.ThemedItem.backgroundLevel
        onPluginSpecChanged: () => {
            pluginDetailsPaneLoader.active = false
            if (pluginSpec !== null) {
                pluginDetailsPaneLoader.active = true
            }
        }
        topPadding: 16
        Loader {
            id: pluginDetailsPaneLoader
            anchors.fill: parent
            active: false
            sourceComponent: PluginDetailsPaneContent {}
        }
    }
    
    QtObject {
        id: d
        property var pluginSpec: null
        onPluginSpecChanged: () => {
            if (view.currentPluginName !== (pluginSpec?.name ?? "")) {
                GlobalHelper.setProperty(view, "currentPluginName", pluginSpec?.name ?? "")
            }
        }
    }

    StackView {
        id: stackView
        visible: !view.useSplitView
        anchors.fill: parent
        initialItem: PluginListView {
        }
        readonly property Item detailsItem: PluginDetailsView {
            StackView.onRemoved: d.pluginSpec = null
        }
        readonly property var pluginSpec: d.pluginSpec
        onPluginSpecChanged: () => {
            if (pluginSpec && currentItem === initialItem) {
                push(detailsItem)
            }
        }
    }

    SplitView {
        id: splitView
        visible: view.useSplitView
        anchors.fill: parent
        PluginListView {
            SplitView.fillHeight: true
            SplitView.preferredWidth: view.navigationWidth
            onWidthChanged: () => {
                if (view.useSplitView)
                    GlobalHelper.setProperty(view, "navigationWidth", width)
            }
        }
        PluginDetailsView {
            SplitView.fillHeight: true
        }
    }
}