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

    // Size units for conversion
    readonly property var sizeUnits: [
        { text: qsTr("KiB"), value: 1024 },
        { text: qsTr("MiB"), value: 1024 * 1024 }
    ]

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
                        onCurrentIndexChanged: page.fileLogLevel = currentIndex
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
                            property int unitIndex: 0 // Default to KiB
                            value: Math.round(page.maxFileSize / page.sizeUnits[unitIndex].value)
                            onValueModified: {
                                page.maxFileSize = value * page.sizeUnits[unitIndex].value
                            }
                        }
                        ComboBox {
                            model: page.sizeUnits
                            textRole: "text"
                            currentIndex: fileSizeSpinBox.unitIndex
                            onCurrentIndexChanged: {
                                let oldValue = fileSizeSpinBox.value * page.sizeUnits[fileSizeSpinBox.unitIndex].value
                                fileSizeSpinBox.unitIndex = currentIndex
                                page.maxFileSize = fileSizeSpinBox.value * page.sizeUnits[currentIndex].value
                            }
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
                            property int unitIndex: 0 // Default to KiB
                            value: Math.round(page.maxArchiveSize / page.sizeUnits[unitIndex].value)
                            onValueModified: {
                                page.maxArchiveSize = value * page.sizeUnits[unitIndex].value
                            }
                        }
                        ComboBox {
                            model: page.sizeUnits
                            textRole: "text"
                            currentIndex: archiveSizeSpinBox.unitIndex
                            onCurrentIndexChanged: {
                                let oldValue = archiveSizeSpinBox.value * page.sizeUnits[archiveSizeSpinBox.unitIndex].value
                                archiveSizeSpinBox.unitIndex = currentIndex
                                page.maxArchiveSize = archiveSizeSpinBox.value * page.sizeUnits[currentIndex].value
                            }
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
                        text: qsTr("Console log level")
                        TextMatcherItem on text { matcher: page.matcher }
                    }
                    ComboBox {
                        Layout.fillWidth: true
                        model: page.logLevels
                        currentIndex: page.consoleLogLevel
                        onCurrentIndexChanged: page.consoleLogLevel = currentIndex
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

            }
        }
    }
}