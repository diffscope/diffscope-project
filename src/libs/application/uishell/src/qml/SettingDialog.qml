import QtQml
import QtQml.Models
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Controls.impl
import QtQuick.Templates as T

import SVSCraft
import SVSCraft.UIComponents
import SVSCraft.UIComponents.impl

import DiffScope.UIShell

Window {
    id: dialog
    flags: Qt.Dialog | Qt.CustomizeWindowHint | Qt.WindowTitleHint | Qt.WindowSystemMenuHint | Qt.WindowCloseButtonHint
    modality: Qt.ApplicationModal
    width: 800
    height: 600
    title: qsTr("Settings")
    property double navigationWidth: 200
    property QtObject settingCatalog: null
    property string currentId: ""
    signal finished()
    function apply() {
        let restoreApplyStatus = false
        for (const page of [...settingCatalogModel.dirtyPages]) {
            if (page.accept()) {
                settingCatalogModel.dirtyPages.delete(page)
                restoreApplyStatus = restoreApplyStatus || page.dirty
            } else {
                showPage(page.id)
                applyFailAnnotation.page = page
                applyFailAnnotation.visible = true
                return false
            }
        }
        applyButton.enabled = restoreApplyStatus
        applyFailAnnotation.visible = false
        return true
    }
    onClosing: () => {
        for (const page of settingCatalogModel.loadedPages) {
            page.endSetting()
        }
        finished()
    }
    onCurrentIdChanged: () => {
        if (currentId !== (settingPageArea.currentPage?.id ?? "")) {
            showPage(currentId)
        }
    }
    function showPage(id) {
        const index = settingCatalogModel.indexForPageId(id)
        if (index.valid) {
            settingPageTree.expandToIndex(index)
            settingPageTree.selectionModel.setCurrentIndex(index, ItemSelectionModel.NoUpdate)
            return true
        }
        return false
    }
    component SettingPageItem: T.TreeViewDelegate {
        id: control

        implicitWidth: leftMargin + __contentIndent + implicitContentWidth + rightPadding + rightMargin
        implicitHeight: 24

        font: Theme.font

        indentation: indicator ? indicator.width : 12
        leftMargin: 4
        rightMargin: 4
        spacing: 4

        topPadding: contentItem ? (height - contentItem.implicitHeight) / 2 : 0
        leftPadding: !mirrored ? leftMargin + __contentIndent : width - leftMargin - __contentIndent - implicitContentWidth

        highlighted: control.selected || control.current
                   || ((control.treeView.selectionBehavior === TableView.SelectRows
                   || control.treeView.selectionBehavior === TableView.SelectionDisabled)
                   && control.row === control.treeView.currentRow)

        text: model.display.title

        Accessible.role: Accessible.TreeItem

        required property int row
        required property var model
        readonly property real __contentIndent: !isTreeNode ? 0 : (depth * indentation) + (indicator ? indicator.width + spacing : 0)

        indicator: Item {
            readonly property real __indicatorIndent: control.leftMargin + (control.depth * control.indentation)
            x: !control.mirrored ? __indicatorIndent : control.width - __indicatorIndent - width
            y: (control.height - height) / 2
            implicitWidth: 16
            implicitHeight: 24
            ColorImage {
                id: arrow
                x: (parent.width - width) / 2
                y: (parent.height - height) / 2
                width: 12
                height: 12
                source: "qrc:/qt/qml/DiffScope/UIShell/assets/ChevronRight12Filled.svg"
                color: !control.enabled ? Theme.foregroundDisabledColorChange.apply(Theme.foregroundPrimaryColor) :
                       control.down ? Theme.foregroundPressedColorChange.apply(Theme.foregroundPrimaryColor) :
                       control.hovered ? Theme.foregroundHoveredColorChange.apply(Theme.foregroundPrimaryColor) :
                       Theme.foregroundPrimaryColor
                transform: [
                    Rotation {
                        origin.x: arrow.width / 2
                        origin.y: arrow.height / 2
                        angle: control.expanded ? 90 : 0
                    },
                    Scale {
                        origin.x: arrow.width / 2
                        origin.y: arrow.height / 2
                        xScale: control.mirrored ? -1 : 1
                    }
                ]
                Behavior on color {
                    ColorAnimation {
                        duration: Theme.colorAnimationDuration
                        easing.type: Easing.OutCubic
                    }
                }
            }
        }

        background: Rectangle {
            implicitHeight: 24
            function transparentIf(condition, color) {
                return condition ? Qt.rgba(color.r, color.g, color.b, 0) : color
            }
            property color _baseColor: control.highlighted ? Theme.accentColor : Theme.buttonColor
            property color _statusColor: !control.enabled ? Theme.controlDisabledColorChange.apply(_baseColor) :
                                       control.down ? Theme.controlPressedColorChange.apply(_baseColor) :
                                       control.hovered ? Theme.controlHoveredColorChange.apply(_baseColor) :
                                       transparentIf(!control.highlighted, _baseColor)
            color: _statusColor

        }

        contentItem: Text {
            clip: false
            text: control.text
            font: control.font
            elide: Text.ElideRight
            color: !control.enabled ? Theme.foregroundDisabledColorChange.apply(Theme.foregroundPrimaryColor) :
                   control.down ? Theme.foregroundPressedColorChange.apply(Theme.foregroundPrimaryColor) :
                   control.hovered ? Theme.foregroundHoveredColorChange.apply(Theme.foregroundPrimaryColor) :
                   control.highlighted ? Theme.foregroundPrimaryColor : Theme.foregroundPrimaryColor
            visible: !control.editing

        }

    }
    SettingCatalogSortFilterProxyModel {
        id: settingCatalogModel
        readonly property string _filterKeyword: searchTextField.text
        property var currentIndex
        property var loadedPages: new Set()
        property var dirtyPages: new Set()
        readonly property Component dirtyChangedConnections: Component {
            Connections {
                function onDirtyChanged() {
                    if (target.dirty) {
                        settingCatalogModel.dirtyPages.add(target)
                        applyButton.enabled = true
                    }
                }
            }
        }
        onCurrentIndexChanged: () => {
            if (currentIndex) {
                const page = data(currentIndex) ?? null
                if (!page)
                    return
                if (!loadedPages.has(page)) {
                    loadedPages.add(page)
                    dirtyChangedConnections.createObject(this, {target: page})
                    page.beginSetting()
                }
                GlobalHelper.setProperty(dialog, "currentId", page.id)
                settingPageArea.currentPage = page
            }
        }
        on_FilterKeywordChanged: () => {
            let switchPage = !settingPageArea.currentPage || !settingPageArea.currentPage.matches(_filterKeyword)
            filterKeyword = _filterKeyword
            noResultFoundLabel.visible = false
            if (switchPage) {
                const index = findFirstMatch()
                if (index.valid) {
                    settingPageTree.expandToIndex(index)
                } else {
                    settingPageArea.currentPage = null
                    noResultFoundLabel.visible = true
                }
                settingPageTree.selectionModel.setCurrentIndex(index, ItemSelectionModel.NoUpdate)
            }
            if (_filterKeyword.length === 0) {
                noResultFoundLabel.visible = false
            }
        }
        sourceModel: SettingCatalogModel {
            settingCatalog: dialog.settingCatalog
        }
        function indexForPageId(id) {
            return mapFromSource(sourceModel.indexForPageId(id))
        }
    }
    Rectangle {
        anchors.fill: parent
        color: Theme.backgroundQuaternaryColor
        Keys.onEscapePressed: dialog.close()
        ColumnLayout {
            anchors.fill: parent
            spacing: 1
            SplitView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Rectangle {
                    SplitView.fillHeight: true
                    SplitView.preferredWidth: dialog.navigationWidth
                    onWidthChanged: GlobalHelper.setProperty(dialog, "navigationWidth", width)
                    color: Theme.backgroundTertiaryColor
                    ColumnLayout {
                        anchors.fill: parent
                        TextField {
                            id: searchTextField
                            Layout.fillWidth: true
                            Layout.margins: 8
                            placeholderText: qsTr("Search")
                            Accessible.name: qsTr("Search")
                            ThemedItem.icon.source: "qrc:/qt/qml/DiffScope/UIShell/assets/Search16Filled.svg"
                        }
                        Label {
                            id: noResultFoundLabel
                            text: qsTr("No result found")
                            Layout.fillWidth: true
                            horizontalAlignment: Text.AlignHCenter
                            ThemedItem.foregroundLevel: SVS.FL_Secondary
                            visible: false
                        }
                        TreeView {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            Accessible.role: Accessible.Tree
                            id: settingPageTree
                            clip: true
                            selectionModel: ItemSelectionModel {
                                onCurrentIndexChanged: () => {
                                    settingCatalogModel.currentIndex = settingCatalogModel.toPersistentModelIndex(currentIndex)
                                }
                            }
                            model: settingCatalogModel
                            delegate: SettingPageItem {
                                onWidthChanged: width = settingPageTree.width
                            }
                        }
                    }
                }
                Rectangle {
                    id: settingPageArea
                    Accessible.role: Accessible.Pane
                    SplitView.fillHeight: true
                    SplitView.fillWidth: true
                    color: Theme.backgroundPrimaryColor
                    property QtObject currentPage: null
                    property Item defaultSettingPageWidget: Item {
                        id: defaultSettingPageWidget
                        anchors.fill: parent
                        anchors.margins: 12
                        property list<QtObject> model: []
                        ColumnLayout {
                            spacing: 4
                            Repeater {
                                model: defaultSettingPageWidget.model
                                delegate: LinkLabel {
                                    required property QtObject modelData
                                    href: modelData.id
                                    linkText: modelData.title
                                    onLinkActivated: (link) => {
                                        searchTextField.text = ""
                                        dialog.showPage(link)
                                    }
                                }
                            }
                        }

                    }
                    property Item compatPageWidget: Item {
                        id: compatPageWidget
                        anchors.fill: parent
                        ColumnLayout {
                            spacing: 4
                            anchors.fill: parent
                            Annotation {
                                id: compatPageAnnotation
                                Layout.leftMargin: 12
                                Layout.rightMargin: 12
                                Layout.fillWidth: true
                                closable: true
                                label: qsTr("This page is in compatibility mode")
                            }
                            Item {
                                id: compatPageWidgetMappingItem
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                            }
                        }
                    }
                    property Item errorPageWidget: Item {
                        id: errorPageWidget
                        anchors.fill: parent
                        ColumnLayout {
                            spacing: 4
                            anchors.fill: parent
                            anchors.margins: 12
                            anchors.topMargin: 0
                            Annotation {
                                Layout.fillWidth: true
                                ThemedItem.controlType: SVS.CT_Error
                                label: qsTr("This settings page cannot be displayed because its type is not supported.")
                            }
                            Item {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                            }
                        }
                    }
                    property SettingPageWidgetDialog settingPageWidgetDialog: SettingPageWidgetDialog {
                        id: settingPageWidgetDialog
                        windowHandle.transientParent: dialog
                        geometry: {
                            void(dialog.x)
                            void(dialog.y)
                            void(compatPageWidgetMappingItem.x)
                            void(compatPageWidgetMappingItem.y)
                            void(compatPageWidget.Window.window)
                            const p = compatPageWidgetMappingItem.mapToGlobal(0, 0)
                            if (widget)
                                widget.geometry = Qt.rect(0, 0, compatPageWidgetMappingItem.width, compatPageWidgetMappingItem.height)
                            return Qt.rect(p.x, p.y, compatPageWidgetMappingItem.width, compatPageWidgetMappingItem.height + (applyFailAnnotation.visible ? applyFailAnnotation.y : 0))
                        }
                    }
                    onCurrentPageChanged: () => {
                        const a = []
                        for (let p = settingPageArea.currentPage; p; p = p.parentPage)
                            a.unshift(p.title)
                        breadcrumbRepeater.model = a
                        settingPageWidgetDialog.hide()
                        if (!currentPage?.widget) {
                            defaultSettingPageWidget.model = currentPage?.pages ?? []
                            settingPageWidgetArea.widget = defaultSettingPageWidget
                        } else if (currentPage.widget instanceof Item) {
                            settingPageWidgetArea.widget = currentPage.widget
                        } else if (settingPageWidgetDialog.isWidget(currentPage.widget)) {
                            settingPageWidgetArea.widget = compatPageWidget
                            compatPageAnnotation.visible = true
                            settingPageWidgetDialog.windowTitle = currentPage.title
                            settingPageWidgetDialog.widget = currentPage.widget
                            settingPageWidgetDialog.show()
                        } else {
                            settingPageWidgetArea.widget = errorPageWidget
                        }
                    }
                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 0
                        ColumnLayout {
                            Layout.fillWidth: true
                            Layout.margins: 12
                            spacing: 4
                            RowLayout {
                                spacing: 4
                                Accessible.role: Accessible.StaticText
                                Accessible.name: breadcrumbRepeater.model.join(" → ")
                                Repeater {
                                    id: breadcrumbRepeater
                                    model: []
                                    delegate: RowLayout {
                                        id: breadcrumbItem
                                        required property string modelData
                                        required property int index
                                        spacing: 4
                                        Label {
                                            Accessible.ignored: true
                                            text: breadcrumbItem.modelData
                                            Layout.alignment: Qt.AlignVCenter
                                            font.weight: Font.DemiBold
                                        }
                                        ColorImage {
                                            height: 12
                                            width: 12
                                            color: Theme.foregroundPrimaryColor
                                            source: "qrc:/qt/qml/DiffScope/UIShell/assets/ChevronRight12Filled.svg"
                                            visible: index !== breadcrumbRepeater.count - 1
                                            Layout.alignment: Qt.AlignVCenter
                                        }
                                    }
                                }
                            }
                            Label {
                                text: settingPageArea.currentPage?.description ?? ""
                            }
                        }
                        Item {
                            id: settingPageWidgetArea
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            property Item widget: null
                            onWidgetChanged: () => {
                                if (widget?.hasOwnProperty("pageMargins")) {
                                    widget.pageMargins = 12
                                }
                            }
                            data: [widget]
                        }
                    }
                }
            }
            Rectangle {
                color: Theme.backgroundSecondaryColor
                Layout.fillWidth: true
                height: 60
                AnnotationPopup {
                    id: applyFailAnnotation
                    ThemedItem.controlType: SVS.CT_Error
                    property QtObject page: null
                    closable: true
                    timeout: 3000
                    title: qsTr("Cannot Apply Settings")
                    content: qsTr('Failed to apply "%1"').arg(page?.title)
                    x: mirrored ? 6 : parent.width - width - 6
                    y: -height - 6
                }
                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 16
                    spacing: 12
                    Item {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                    }
                    Button {
                        ThemedItem.controlType: SVS.CT_Accent
                        Layout.alignment: Qt.AlignVCenter
                        text: qsTr("OK")
                        Component.onCompleted: forceActiveFocus()
                        onClicked: () => {
                            if (dialog.apply()) {
                                dialog.close()
                            }
                        }
                    }
                    Button {
                        Layout.alignment: Qt.AlignVCenter
                        text: qsTr("Cancel")
                        onClicked: () => {
                            dialog.close()
                        }
                    }
                    Button {
                        id: applyButton
                        Layout.alignment: Qt.AlignVCenter
                        enabled: false
                        text: qsTr("Apply")
                        onClicked: dialog.apply()
                    }
                }
            }
        }
    }
}