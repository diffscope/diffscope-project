import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts

import SVSCraft
import SVSCraft.UIComponents

import DiffScope.PackageManager

ScrollView {
    id: page

    required property QtObject pageHandle
    property bool started: false
    property string dspmPath: ""
    property string packageDir: ""
    property int timeoutSeconds: 5

    onDspmPathChanged: if (started) pageHandle.markDirty()
    onPackageDirChanged: if (started) pageHandle.markDirty()
    onTimeoutSecondsChanged: if (started) pageHandle.markDirty()

    anchors.fill: parent
    contentWidth: availableWidth

    readonly property TextMatcher matcher: TextMatcher {}

    FileDialog {
        id: dspmFileDialog
        title: qsTr("Select Package Manager")
        fileMode: FileDialog.OpenFile
        nameFilters: [qsTr("Executable files (*)")]
        onAccepted: {
            if (page.pageHandle.validateDspmPath(selectedFile, page.timeoutSeconds)) {
                page.dspmPath = page.pageHandle.localFilePath(selectedFile)
            }
        }
    }

    FolderDialog {
        id: packageDirDialog
        title: qsTr("Select Package Directory")
        onAccepted: page.packageDir = page.pageHandle.localFilePath(selectedFolder)
    }

    ColumnLayout {
        width: page.width

        ColumnLayout {
            Layout.fillWidth: true
            Layout.margins: 12
            spacing: 32

            GroupBox {
                title: qsTr("Package Manager")
                TextMatcherItem on title { matcher: page.matcher }
                Layout.fillWidth: true

                GridLayout {
                    anchors.fill: parent
                    columns: 3

                    Label {
                        text: qsTr("Command")
                        TextMatcherItem on text { matcher: page.matcher }
                    }
                    TextField {
                        Layout.fillWidth: true
                        readOnly: true
                        text: page.dspmPath
                    }
                    ToolButton {
                        text: qsTr("Browse")
                        icon.source: "image://fluent-system-icons/folder_open"
                        display: AbstractButton.IconOnly
                        onClicked: dspmFileDialog.open()
                    }

                    Label {
                        text: qsTr("Package directory")
                        TextMatcherItem on text { matcher: page.matcher }
                    }
                    TextField {
                        Layout.fillWidth: true
                        text: page.packageDir
                        onTextEdited: page.packageDir = text
                    }
                    ToolButton {
                        text: qsTr("Browse")
                        icon.source: "image://fluent-system-icons/folder_open"
                        display: AbstractButton.IconOnly
                        onClicked: packageDirDialog.open()
                    }

                    Label {
                        text: qsTr("Command timeout")
                        TextMatcherItem on text { matcher: page.matcher }
                    }
                    RowLayout {
                        Layout.columnSpan: 2
                        SpinBox {
                            from: 1
                            to: 300
                            value: page.timeoutSeconds
                            editable: true
                            onValueModified: page.timeoutSeconds = value
                        }
                        Label {
                            text: qsTr("seconds")
                            ThemedItem.foregroundLevel: SVS.FL_Secondary
                        }
                    }
                }
            }

            Item {
                Layout.fillHeight: true
            }
        }
    }
}
