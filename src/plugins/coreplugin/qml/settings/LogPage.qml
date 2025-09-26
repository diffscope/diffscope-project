import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SVSCraft
import SVSCraft.UIComponents

import DiffScope.Core

ScrollView {
    id: page

    required property QtObject pageHandle
    property bool started: false
    property var maxFileSize: 0
    property var maxArchiveSize: 0
    property int maxArchiveDays: 0
    property bool prettifiesConsoleOutput: false
    property int consoleLogLevel: 1 // Info
    property int fileLogLevel: 1 // Info
    property int compressLevel: 1

    // Log level options
    readonly property var logLevels: [
        qsTr("Debug"),
        qsTr("Info"),
        qsTr("Warning"),
        qsTr("Critical"),
        qsTr("Fatal"),
    ]

    // Mark dirty when properties change
    onMaxFileSizeChanged: if (started) pageHandle.markDirty()
    onMaxArchiveSizeChanged: if (started) pageHandle.markDirty()
    onMaxArchiveDaysChanged: if (started) pageHandle.markDirty()
    onPrettifiesConsoleOutputChanged: if (started) pageHandle.markDirty()
    onConsoleLogLevelChanged: if (started) pageHandle.markDirty()
    onFileLogLevelChanged: if (started) pageHandle.markDirty()
    onCompressLevelChanged: if (started) pageHandle.markDirty()

    anchors.fill: parent
    contentWidth: availableWidth

    readonly property TextMatcher matcher: TextMatcher {}

    ColumnLayout {
        width: page.width
        ColumnLayout {
            Layout.fillWidth: true
            Layout.margins: 12
            spacing: 32

            GroupBox {
                title: qsTr("File Logging")
                TextMatcherItem on title { matcher: page.matcher }
                Layout.fillWidth: true
                
                GridLayout {
                    anchors.fill: parent
                    columns: 2

                    Label {
                        text: qsTr("File log level")
                        TextMatcherItem on text { matcher: page.matcher }
                    }
                    ComboBox {
                        Layout.fillWidth: true
                        model: page.logLevels
                        currentIndex: page.fileLogLevel
                        onActivated: (index) => page.fileLogLevel = index
                    }

                    Label {
                        text: qsTr("Max file size")
                        TextMatcherItem on text { matcher: page.matcher }
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        SpinBox {
                            id: fileSizeSpinBox
                            from: 1
                            to: 2147483647
                            value: Math.round(page.maxFileSize / 1024)
                            onValueModified: {
                                page.maxFileSize = value * 1024
                            }
                        }
                        Label {
                            text: qsTr("KiB")
                        }
                    }

                    Label {
                        text: qsTr("Max archive size")
                        TextMatcherItem on text { matcher: page.matcher }
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        SpinBox {
                            id: archiveSizeSpinBox
                            from: 1
                            to: 2147483647
                            value: Math.round(page.maxArchiveSize / 1024)
                            onValueModified: {
                                page.maxArchiveSize = value * 1024
                            }
                        }
                        Label {
                            text: qsTr("KiB")
                        }
                    }

                    Label {
                        text: qsTr("Max archive days")
                        TextMatcherItem on text { matcher: page.matcher }
                    }
                    SpinBox {
                        from: 1
                        to: 3650 // ~10 years
                        value: page.maxArchiveDays
                        onValueModified: page.maxArchiveDays = value
                    }

                    Label {
                        text: qsTr("Compress level")
                        TextMatcherItem on text { matcher: page.matcher }
                    }
                    RowLayout {
                        SpinBox {
                            from: 0
                            to: 9
                            value: page.compressLevel
                            onValueModified: page.compressLevel = value
                        }
                        Label {
                            ThemedItem.foregroundLevel: SVS.FL_Secondary
                            text: qsTr("(0 = no compress, 9 = best compress)")
                        }
                    }
                }
            }

            GroupBox {
                title: qsTr("Console Logging")
                TextMatcherItem on title { matcher: page.matcher }
                Layout.fillWidth: true
                
                GridLayout {
                    anchors.fill: parent
                    columns: 2
                    Label {
                        visible: page.pageHandle.debugMode
                        Layout.columnSpan: 2
                        text: qsTr('Console log level is overridden to "Debug" in a debug build')
                    }
                    Label {
                        text: qsTr("Console log level")
                        TextMatcherItem on text { matcher: page.matcher }
                        enabled: !page.pageHandle.debugMode
                    }
                    ComboBox {
                        Layout.fillWidth: true
                        model: page.logLevels
                        enabled: !page.pageHandle.debugMode
                        currentIndex: page.pageHandle.debugMode ? 0 : page.consoleLogLevel
                        onActivated: (index) => page.consoleLogLevel = index
                    }

                    CheckBox {
                        Layout.columnSpan: 2
                        checked: page.prettifiesConsoleOutput
                        onClicked: page.prettifiesConsoleOutput = checked
                        text: qsTr("Prettify console output")
                        TextMatcherItem on text { matcher: page.matcher }
                    }
                }
            }

            GroupBox {
                title: qsTr("Log Location")
                TextMatcherItem on title { matcher: page.matcher }
                Layout.fillWidth: true
                ColumnLayout {
                    anchors.fill: parent
                    TextEdit {
                        text: page.pageHandle.logsLocation
                        Layout.fillWidth: true
                        wrapMode: Text.WrapAnywhere
                        readOnly: true
                        color: Theme.foregroundColor(ThemedItem.foregroundLevel)
                        Accessible.role: Accessible.StaticText
                        Accessible.name: text
                        selectionColor: Theme.accentColor
                    }
                    Button {
                        text: qsTr("Reveal in %1").replace("%1", DesktopServices.fileManagerName)
                        onClicked: DesktopServices.reveal(page.pageHandle.logsLocation)
                    }
                }
            }
        }
    }
}