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
    property bool useSplitView: true
    clip: true

    property var model: null
    property var currentIndex: packageListProxyModel.invalidIndex()
    property Component singerExtraDelegate: null

    PackageListProxyModel {
        id: packageListProxyModel
        sourceModel: view.model
    }

    component PackageCard: T.Button {
        id: packageCard
        required property var modelData
        required property int index
        required property Repeater repeater
        Accessible.role: Accessible.ListItem
        Layout.fillWidth: true
        height: 60
        onClicked: () => {
            view.currentIndex = packageListProxyModel.entryIndex(packageListProxyModel.packageModelIndex(packageCard.index))
        }
        Card {
            anchors.fill: parent
            atTop: packageCard.index === 0
            atBottom: packageCard.index === packageCard.repeater.count - 1
            property color _baseColor: view.useSplitView ? Theme.backgroundTertiaryColor : Theme.backgroundColor(view.ThemedItem.backgroundLevel)
            property color color: packageCard.pressed ? Theme.controlPressedColorChange.apply(_baseColor) : packageCard.hovered ? Theme.controlHoveredColorChange.apply(_baseColor) : _baseColor
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
                    text: packageCard.modelData.name || packageCard.modelData.id
                    font.weight: Font.DemiBold
                }
                Label {
                    text: packageCard.modelData.version
                    ThemedItem.foregroundLevel: SVS.FL_Secondary
                }
            }
            subtitle: packageCard.modelData.vendor
            image: Rectangle {
                color: Theme.backgroundQuaternaryColor
                IconLabel {
                    anchors.centerIn: parent
                    icon.source: "image://fluent-system-icons/box?size=32&style=regular"
                    icon.color: Theme.foregroundSecondaryColor
                    icon.width: 32
                    icon.height: 32
                }
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
        property var toolsModel: null
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
            Repeater {
                model: card.toolsModel
            }
        }
    }

    component PackageDetailsPaneContent: ColumnLayout {
        id: pane
        spacing: 16
        required property var modelData
        required property int index
        readonly property var modelIndex: packageListProxyModel.packageModelIndex(index)
        ColumnLayout {
            Layout.fillWidth: true
            Layout.leftMargin: 16
            Layout.rightMargin: 16
            spacing: 16
            RowLayout {
                spacing: 8
                Layout.maximumHeight: implicitHeight
                Button {
                    icon.source: "image://fluent-system-icons/arrow_left?size=24"
                    text: qsTr("Back")
                    flat: true
                    display: AbstractButton.IconOnly
                    onClicked: view.currentIndex = packageListProxyModel.invalidIndex()
                }
                SelectableLabel {
                    text: pane.modelData.name || pane.modelData.id
                    font.pixelSize: 24
                    font.weight: Font.Medium

                    DescriptiveText.toolTip: qsTr("Package name")
                    Accessible.description: DescriptiveText.toolTip
                    DescriptiveText.activated: packageNameHoverHandler.hovered
                    HoverHandler {
                        id: packageNameHoverHandler
                    }
                }
                Item {
                    Layout.fillWidth: true
                }
                Button {
                    text: qsTr("Uninstall")
                    icon.source: "image://fluent-system-icons/delete"
                }
            }
            RowLayout {
                spacing: 16
                Layout.maximumHeight: implicitHeight
                SelectableLabel {
                    text: pane.modelData.vendor
                    DescriptiveText.toolTip: qsTr("Vendor")
                    Accessible.description: DescriptiveText.toolTip
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
                    text: pane.modelData.version
                    DescriptiveText.toolTip: qsTr("Version")
                    Accessible.description: DescriptiveText.toolTip
                    DescriptiveText.activated: versionHoverHandler.hovered
                    HoverHandler {
                        id: versionHoverHandler
                    }
                }
                Rectangle {
                    Layout.fillHeight: true
                    width: 1
                    color: Theme.foregroundSecondaryColor
                    visible: Boolean(pane.modelData.url)
                }
                LinkLabel {
                    visible: href.length !== 0
                    href: pane.modelData.url ?? ""
                    linkText: qsTr("Visit package homepage")
                    externalLink: true
                    onLinkActivated: (link) => Qt.openUrlExternally(link)
                }
            }
            SelectableLabel {
                text: pane.modelData.description
                Layout.maximumWidth: packageDetailsPane.width - 32
                DescriptiveText.toolTip: qsTr("Description")
                Accessible.description: DescriptiveText.toolTip
                DescriptiveText.activated: descriptionHoverHandler.hovered
                wrapMode: Text.Wrap
                HoverHandler {
                    id: descriptionHoverHandler
                }
            }
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 0
                TabBar {
                    id: tabBar
                    ThemedItem.flat: true
                    currentIndex: {
                        if (packageListProxyModel.isSingerOrSingerRootIndex(view.currentIndex))
                            return 0
                        if (packageListProxyModel.isInferenceOrInferenceRootIndex(view.currentIndex))
                            return 1
                        if (packageListProxyModel.isPackageIndex(view.currentIndex))
                            return 2
                        if (packageListProxyModel.isDependencyRootIndex(view.currentIndex))
                            return 3
                        return -1
                    }
                    TabButton {
                        text: qsTr("Singers (%L1)").arg(singerListRepeater.count)
                        ThemedItem.tabIndicator: SVS.TI_Bottom
                        width: implicitWidth
                        onClicked: view.currentIndex = packageListProxyModel.singerModelIndexForIndex(view.currentIndex)
                    }
                    TabButton {
                        text: qsTr("Inferences (%L1)").arg(inferenceListRepeater.count)
                        ThemedItem.tabIndicator: SVS.TI_Bottom
                        width: implicitWidth
                        onClicked: view.currentIndex = packageListProxyModel.inferenceRootIndexForIndex(view.currentIndex)
                    }
                    TabButton {
                        text: qsTr("Details")
                        ThemedItem.tabIndicator: SVS.TI_Bottom
                        width: implicitWidth
                        onClicked: view.currentIndex = packageListProxyModel.packageModelIndexForIndex(view.currentIndex)
                    }
                    TabButton {
                        text: qsTr("Dependencies")
                        ThemedItem.tabIndicator: SVS.TI_Bottom
                        width: implicitWidth
                        onClicked: view.currentIndex = packageListProxyModel.dependencyRootIndexForIndex(view.currentIndex)
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
            Item {
                Label {
                    anchors.left: parent.left
                    anchors.leftMargin: 16
                    text: qsTr("This package does not contain any singers.")
                    visible: singerListRepeater.count === 0
                }
                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 16
                    anchors.topMargin: 0
                    spacing: 0
                    visible: singerListRepeater.count !== 0
                    ScrollView {
                        id: singerListScrollView
                        Layout.fillHeight: true
                        ColumnLayout {
                            id: singerListScrollViewContent
                            spacing: 8
                            Repeater {
                                id: singerListRepeater
                                model: DelegateModel {
                                    id: singerListModel
                                    model: packageListProxyModel
                                    rootIndex: packageListProxyModel.singerRootIndexForIndex(pane.modelIndex)
                                    delegate: T.Button {
                                        id: singerButton
                                        required property var modelData
                                        required property int index
                                        implicitWidth: implicitContentWidth + leftPadding + rightPadding
                                        implicitHeight: implicitContentHeight + topPadding + bottomPadding
                                        rightPadding: 8
                                        text: modelData.name || modelData.id
                                        contentItem: ColumnLayout {
                                            Rectangle {
                                                width: 66
                                                height: 66
                                                Layout.alignment: Qt.AlignHCenter
                                                color: Theme.backgroundTertiaryColor
                                                border.width: 1
                                                border.color: singerButton.highlighted ? Theme.accentColor : Theme.borderColor
                                                IconImage {
                                                    anchors.centerIn: parent
                                                    source: "image://fluent-system-icons/mic?size=48&style=regular"
                                                    color: Theme.foregroundSecondaryColor
                                                    sourceSize.width: 48
                                                    sourceSize.height: 48
                                                    visible: !singerButton.modelData.avatarPath
                                                }
                                                Image {
                                                    width: 64
                                                    height: 64
                                                    anchors.centerIn: parent
                                                    source: singerButton.modelData.avatarPath ?? ""
                                                }
                                            }
                                            Text {
                                                text: singerButton.text
                                                font: Theme.font
                                                color: singerButton.highlighted ? Theme.accentColor : Theme.foregroundPrimaryColor
                                                wrapMode: Text.Wrap
                                                Layout.maximumWidth: 66
                                                Layout.alignment: Qt.AlignHCenter
                                                horizontalAlignment: Text.AlignHCenter

                                            }
                                        }
                                        onClicked: view.currentIndex = packageListProxyModel.index(index, 0, singerListModel.rootIndex)
                                        highlighted: view.currentIndex.row === index
                                    }
                                }
                            }
                        }
                    }
                    Rectangle {
                        Layout.fillHeight: true
                        width: 1
                        color: Theme.borderColor
                    }
                    StackLayout {
                        Layout.fillHeight: true
                        Layout.fillWidth: true
                        currentIndex: view.currentIndex.row
                        Repeater {
                            model: DelegateModel {
                                model: packageListProxyModel
                                rootIndex: packageListProxyModel.singerRootIndexForIndex(pane.modelIndex)
                                delegate: RowLayout {
                                    id: singerDetailView
                                    required property var modelData
                                    required property int index
                                    readonly property var modelIndex: packageListProxyModel.index(index, 0, packageListProxyModel.singerRootIndexForIndex(pane.modelIndex))
                                    spacing: 8
                                    ScrollView {
                                        id: detailsScrollView
                                        contentWidth: availableWidth
                                        Layout.fillWidth: true
                                        Layout.fillHeight: true
                                        ColumnLayout {
                                            width: detailsScrollView.width
                                            ColumnLayout {
                                                Layout.fillWidth: true
                                                Layout.leftMargin: 8
                                                spacing: 8
                                                RowLayout {
                                                    Layout.fillWidth: true
                                                    spacing: 8
                                                    Rectangle {
                                                        width: 66
                                                        height: 66
                                                        Layout.alignment: Qt.AlignHCenter
                                                        color: Theme.backgroundTertiaryColor
                                                        border.width: 1
                                                        border.color: Theme.borderColor
                                                        IconImage {
                                                            anchors.centerIn: parent
                                                            source: "image://fluent-system-icons/mic?size=48&style=regular"
                                                            color: Theme.foregroundSecondaryColor
                                                            sourceSize.width: 48
                                                            sourceSize.height: 48
                                                            visible: !singerDetailView.modelData.avatarPath
                                                        }
                                                        Image {
                                                            width: 64
                                                            height: 64
                                                            anchors.centerIn: parent
                                                            source: singerDetailView.modelData.avatarPath ?? ""
                                                        }
                                                    }
                                                    ColumnLayout {
                                                        Layout.fillWidth: true
                                                        SelectableLabel {
                                                            text: singerDetailView.modelData.name || singerDetailView.modelData.id
                                                            font.pixelSize: 20
                                                            font.weight: Font.Medium

                                                            DescriptiveText.toolTip: qsTr("Singer name")
                                                            Accessible.description: DescriptiveText.toolTip
                                                            DescriptiveText.activated: singerNameHoverHandler.hovered
                                                            HoverHandler {
                                                                id: singerNameHoverHandler
                                                            }
                                                        }
                                                        Flow {
                                                            Layout.fillWidth: true
                                                            spacing: 8
                                                            Repeater {
                                                                model: DelegateModel {
                                                                    model: packageListProxyModel
                                                                    rootIndex: packageListProxyModel.demoAudioRootIndexForSingerIndex(singerDetailView.modelIndex)
                                                                    delegate: Button {
                                                                        required property var modelData
                                                                        icon.source: "image://fluent-system-icons/play_circle"
                                                                        text: modelData.name
                                                                        // TODO play audio
                                                                    }
                                                                }
                                                            }
                                                        }
                                                    }
                                                }
                                                InfoCard {
                                                    title: qsTr("Identifier")
                                                    text: singerDetailView.modelData.id
                                                }
                                                InfoCard {
                                                    title: qsTr("Singer class")
                                                    text: singerDetailView.modelData.className
                                                }
                                                StackLayout {
                                                    Layout.fillWidth: true
                                                    implicitHeight: item ? item.implicitHeight : 0
                                                    property Item item: null
                                                    readonly property Component delegate: view.singerExtraDelegate
                                                    onDelegateChanged: () => {
                                                        if (item) {
                                                            item.destroy()
                                                        }
                                                        item = delegate.createObject(this, {
                                                            modelData: singerDetailView.modelData,
                                                            index: singerDetailView.index
                                                        })
                                                    }
                                                    data: [item]
                                                }
                                                GroupBox {
                                                    id: advancedGroupBox
                                                    title: qsTr("Advanced")
                                                    Layout.fillWidth: true
                                                    ColumnLayout {
                                                        anchors.fill: parent
                                                        spacing: 8
                                                        InfoCard {
                                                            title: qsTr("Configuration file")
                                                            text: singerDetailView.modelData.path
                                                            toolsModel: ObjectModel {
                                                                ToolButton {
                                                                    icon.source: "image://fluent-system-icons/open"
                                                                    display: AbstractButton.IconOnly
                                                                    text: qsTr("Reveal in %1").arg(DesktopServices.fileManagerName)
                                                                    onClicked: () => {
                                                                        DesktopServices.reveal(singerDetailView.modelData.path)
                                                                    }
                                                                }
                                                            }
                                                        }
                                                        Repeater {
                                                            model: DelegateModel {
                                                                model: packageListProxyModel
                                                                rootIndex: packageListProxyModel.importRootIndexForSingerIndex(singerDetailView.modelIndex)
                                                                delegate: InfoCard {
                                                                    id: importedInferenceCard
                                                                    required property var modelData
                                                                    title: qsTr("Imported inference model")
                                                                    text: {
                                                                        let refText = `${modelData.id}@${modelData.version}:${modelData.importInferenceId}`
                                                                        if (modelData.name)
                                                                            return qsTr("%1 (%2)").arg(modelData.name).arg(refText)
                                                                        return refText
                                                                    }
                                                                    toolsModel: ObjectModel {
                                                                        ToolButton {
                                                                            icon.source: "image://fluent-system-icons/open"
                                                                            display: AbstractButton.IconOnly
                                                                            text: qsTr("Jump to details")
                                                                            onClicked: () => {
                                                                                view.currentIndex = packageListProxyModel.inferenceRootIndexForIndex(packageListProxyModel.findInferenceIndex(importedInferenceCard.modelData.id, importedInferenceCard.modelData.version, importedInferenceCard.modelData.importInferenceId))
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
                                    }
                                    Item {
                                        Layout.preferredWidth: 240
                                        Layout.fillHeight: true
                                        visible: Boolean(singerDetailView.modelData.backgroundPath)
                                        GroupBox {
                                            title: qsTr("Portrait")
                                            anchors.fill: parent
                                            Frame {
                                                anchors.fill: parent
                                                ThemedItem.backgroundLevel: SVS.BL_Tertiary
                                                Image {
                                                    anchors.fill: parent
                                                    anchors.margins: 8
                                                    source: singerDetailView.modelData.backgroundPath ?? ""
                                                    fillMode: Image.PreserveAspectFit
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
            Item {
                Label {
                    anchors.left: parent.left
                    anchors.leftMargin: 16
                    text: qsTr("This package does not contain any inference modules.")
                    visible: inferenceListRepeater.count === 0
                }
                ScrollView {
                    id: inferenceListScrollView
                    contentWidth: availableWidth
                    visible: inferenceListRepeater.count !== 0
                    anchors.fill: parent
                    ColumnLayout {
                        width: inferenceListScrollView.width
                        ColumnLayout {
                            Layout.fillWidth: true
                            Layout.leftMargin: 16
                            Layout.rightMargin: 16
                            Layout.bottomMargin: 16
                            spacing: 16
                            Repeater {
                                id: inferenceListRepeater
                                model: DelegateModel {
                                    model: packageListProxyModel
                                    rootIndex: packageListProxyModel.inferenceRootIndexForIndex(pane.modelIndex)
                                    delegate: InfoCard {
                                        id: inferenceCard
                                        required property var modelData
                                        title: {
                                            if (modelData.name)
                                                return qsTr("%1 (%2)").arg(modelData.name).arg(modelData.id)
                                            return modelData.id
                                        }
                                        text: modelData.path
                                        toolsModel: ObjectModel {
                                            ToolButton {
                                                icon.source: "image://fluent-system-icons/open"
                                                display: AbstractButton.IconOnly
                                                text: qsTr("Reveal in %1").arg(DesktopServices.fileManagerName)
                                                onClicked: () => {
                                                    DesktopServices.reveal(inferenceCard.modelData.path)
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
            ScrollView {
                id: detailsScrollView
                contentWidth: availableWidth
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
                            text: pane.modelData.id
                        }
                        InfoCard {
                            title: qsTr("Readme file")
                            text: pane.modelData.readmePath ?? ""
                            visible: Boolean(pane.modelData.readmePath)
                            toolsModel: ObjectModel {
                                ToolButton {
                                    icon.source: "image://fluent-system-icons/open"
                                    display: AbstractButton.IconOnly
                                    text: qsTr("Open in %1").arg(DesktopServices.fileManagerName)
                                    onClicked: () => {
                                        DesktopServices.reveal(pane.modelData.readmePath)
                                    }
                                }
                            }
                        }
                        InfoCard {
                            title: qsTr("License file")
                            text: pane.modelData.licensePath ?? ""
                            visible: Boolean(pane.modelData.licensePath)
                            toolsModel: ObjectModel {
                                ToolButton {
                                    icon.source: "image://fluent-system-icons/open"
                                    display: AbstractButton.IconOnly
                                    text: qsTr("Open in %1").arg(DesktopServices.fileManagerName)
                                    onClicked: () => {
                                        DesktopServices.reveal(pane.modelData.licensePath)
                                    }
                                }
                            }
                        }
                        InfoCard {
                            title: qsTr("Installation time")
                            text: (new Date(pane.modelData.installationTime)).toLocaleString()
                        }
                        InfoCard {
                            title: qsTr("Installation location")
                            text: pane.modelData.path
                            toolsModel: ObjectModel {
                                ToolButton {
                                    icon.source: "image://fluent-system-icons/open"
                                    display: AbstractButton.IconOnly
                                    text: qsTr("Open in %1").arg(DesktopServices.fileManagerName)
                                    onClicked: () => {
                                        DesktopServices.reveal(pane.modelData.path)
                                    }
                                }
                            }
                        }
                    }
                }
            }
            ScrollView {
                id: dependencyScrollView
                contentWidth: availableWidth
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
                            text: qsTr("This package does not depend on any other packages.")
                            visible: dependencyRepeater.count === 0
                        }
                        Repeater {
                            id: dependencyRepeater
                            // TODO
                        }
                    }
                }
            }
        }
    }
    component PackageListView : Rectangle {
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
                    ThemedItem.icon.source: "image://fluent-system-icons/search"
                }
            }

            ScrollView {
                id: packagesScrollView
                contentWidth: availableWidth
                Layout.fillWidth: true
                Layout.fillHeight: true
                ColumnLayout {
                    width: packagesScrollView.width
                    spacing: 0
                    Label {
                        text: qsTr("No result found")
                        ThemedItem.foregroundLevel: SVS.FL_Secondary
                        Layout.alignment: Qt.AlignHCenter
                        visible: packageCollectionLayout.visibleChildren.length === 1
                    }
                    ColumnLayout {
                        id: packageCollectionLayout
                        spacing: 0
                        Layout.fillWidth: true
                        Layout.leftMargin: 8
                        Layout.rightMargin: 8
                        Repeater {
                            id: packageCollectionRepeater
                            model: packageListProxyModel
                            delegate: PackageCard {
                                repeater: packageCollectionRepeater
                            }
                        }
                    }
                }
            }
        }
    }
    component PackageDetailsView: Pane {
        id: packageDetailsPane
        ThemedItem.backgroundLevel: view.ThemedItem.backgroundLevel
        topPadding: 16
        StackLayout {
            currentIndex: packageListProxyModel.packageIndexForIndex(view.currentIndex) + 1
            anchors.fill: parent
            Item {}
            Repeater {
                model: packageListProxyModel
                PackageDetailsPaneContent {}
            }
        }
    }

    StackView {
        id: stackView
        visible: !view.useSplitView
        anchors.fill: parent
        initialItem: PackageListView {
        }
        readonly property Item detailsItem: PackageDetailsView {
            StackView.onRemoved: () => {
                // TODO
            }
        }
    }

    SplitView {
        id: splitView
        visible: view.useSplitView
        anchors.fill: parent
        PackageListView {
            SplitView.fillHeight: true
            SplitView.preferredWidth: view.navigationWidth
            onWidthChanged: () => {
                if (view.useSplitView)
                    GlobalHelper.setProperty(view, "navigationWidth", width)
            }
        }
        PackageDetailsView {
            SplitView.fillHeight: true
        }
    }
}