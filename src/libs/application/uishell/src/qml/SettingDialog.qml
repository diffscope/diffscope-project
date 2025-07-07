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
    flags: Qt.Dialog
    width: 800
    height: 600
    title: qsTr("Settings")
    property double navigationWidth: 200
    property QtObject settingCatalog: null
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

        contentItem: Label {
            clip: false
            text: control.text
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
        onCurrentIndexChanged: () => {
            if (currentIndex)
                settingPageArea.currentPage = data(currentIndex) ?? null
        }
        on_FilterKeywordChanged: () => {
            let switchPage = !settingPageArea.currentPage || !settingPageArea.currentPage.matches(_filterKeyword)
            filterKeyword = _filterKeyword
            if (switchPage) {
                const index = findFirstMatch()
                if (index.valid) {
                    settingPageTree.expandToIndex(index)

                } else {
                    settingPageArea.currentPage = null
                }
                settingPageTree.selectionModel.setCurrentIndex(index, ItemSelectionModel.NoUpdate)
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
                            ThemedItem.icon.source: "qrc:/qt/qml/DiffScope/UIShell/assets/Search16Filled.svg"
                        }
                        TreeView {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
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
                    SplitView.fillHeight: true
                    SplitView.fillWidth: true
                    color: Theme.backgroundPrimaryColor
                    property QtObject currentPage: null
                    property Item defaultSettingPageWidget: Item {
                        id: defaultSettingPageWidget
                        anchors.fill: parent
                        anchors.margins: 12
                        anchors.topMargin: 0
                        property list<QtObject> model: []
                        ColumnLayout {
                            spacing: 4
                            Repeater {
                                model: defaultSettingPageWidget.model
                                delegate: Label {
                                    required property QtObject modelData
                                    textFormat: Text.RichText
                                    text: `<a href="${modelData.id}" style="text-decoration: ${hoveredLink ? "underline" : "none"}; color: rgba(${Theme.linkColor.r * 255}, ${Theme.linkColor.g * 255}, ${Theme.linkColor.b * 255}, ${Theme.linkColor.a});">${modelData.title}</a>`
                                    onLinkActivated: (link) => {
                                        searchTextField.text = ""
                                        dialog.showPage(link)
                                    }
                                }
                            }
                        }

                    }
                    onCurrentPageChanged: () => {
                        const a = []
                        for (let p = settingPageArea.currentPage; p; p = p.parentPage)
                            a.unshift(p.title)
                        breadcrumbRepeater.model = a
                        defaultSettingPageWidget.model = currentPage?.pages ?? []
                        settingPageWidgetArea.widget = currentPage?.widget instanceof Item ? currentPage.widget : defaultSettingPageWidget
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
                                Repeater {
                                    id: breadcrumbRepeater
                                    model: []
                                    delegate: RowLayout {
                                        id: breadcrumbItem
                                        required property string modelData
                                        required property int index
                                        spacing: 4
                                        Label {
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
                        text: qsTr("OK")
                    }
                    Button {
                        text: qsTr("Cancel")
                    }
                    Button {
                        text: qsTr("Apply")
                    }
                }
            }
        }
    }
}