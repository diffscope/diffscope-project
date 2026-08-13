import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl
import QtQuick.Layouts

import SVSCraft
import SVSCraft.UIComponents
import SVSCraft.UIComponents.impl

Item {
    id: page

    required property QtObject pageHandle
    required property var configurationModel
    property bool started: false
    property bool advancedVisible: false
    readonly property TextMatcher matcher: TextMatcher {}
    readonly property var currentService: serviceList.currentItem?.serviceModel ?? null

    anchors.fill: parent

    function addService() {
        const index = page.configurationModel.addService()
        serviceList.currentIndex = index
        serviceList.positionViewAtIndex(index, ListView.Contain)
    }

    function removeCurrentService() {
        const index = serviceList.currentIndex
        if (!page.configurationModel.removeService(index))
            return
        serviceList.currentIndex = Math.min(index, serviceList.count - 1)
    }

    function moveCurrentService(offset) {
        const index = serviceList.currentIndex
        if (!page.configurationModel.moveService(index, offset))
            return
        serviceList.currentIndex = index + offset
        serviceList.positionViewAtIndex(serviceList.currentIndex, ListView.Contain)
    }

    onCurrentServiceChanged: advancedVisible = false

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Label {
            Layout.fillWidth: true
            Layout.leftMargin: 12
            Layout.rightMargin: 12
            Layout.bottomMargin: visible ? 8 : 0
            visible: text.length > 0
            text: page.pageHandle.errorMessage
            color: Theme.errorColor
            wrapMode: Text.Wrap
        }

        SplitView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.margins: 12

            Frame {
                SplitView.preferredWidth: 220
                SplitView.minimumWidth: 220
                SplitView.maximumWidth: 400
                padding: 1

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 0

                    ListView {
                        id: serviceList
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        focus: true
                        model: page.configurationModel
                        currentIndex: -1

                        onCountChanged: {
                            if (count === 0)
                                currentIndex = -1
                            else if (currentIndex < 0)
                                currentIndex = 0
                            else if (currentIndex >= count)
                                currentIndex = count - 1
                        }

                        delegate: ItemDelegate {
                            id: serviceDelegate
                            required property var model
                            required property int index
                            property var serviceModel: model

                            width: ListView.view.width
                            padding: 4
                            leftPadding: 8
                            rightPadding: 8
                            highlighted: ListView.isCurrentItem
                            ThemedItem.flat: true
                            ThemedItem.controlType: highlighted ? SVS.CT_Accent : SVS.CT_Normal
                            background: ButtonRectangle {
                                control: serviceDelegate
                                checked: serviceDelegate.highlighted
                                flat: true
                            }
                            contentItem: RowLayout {
                                spacing: 8
                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 1
                                    Label {
                                        Layout.fillWidth: true
                                        text: serviceDelegate.model.name.length > 0
                                              ? serviceDelegate.model.name
                                              : qsTr("Unnamed service")
                                        font.weight: Font.DemiBold
                                        elide: Text.ElideRight
                                    }
                                    Label {
                                        Layout.fillWidth: true
                                        text: serviceDelegate.model.baseUrl
                                        elide: Text.ElideMiddle
                                        ThemedItem.foregroundLevel: SVS.FL_Secondary
                                    }
                                }
                                Label {
                                    visible: !serviceDelegate.model.enabled
                                    text: qsTr("Disabled")
                                    ThemedItem.foregroundLevel: SVS.FL_Secondary
                                }
                            }
                            onClicked: serviceList.currentIndex = index
                        }

                        Label {
                            anchors.centerIn: parent
                            visible: serviceList.count === 0
                            text: qsTr("No DSSP services configured")
                            ThemedItem.foregroundLevel: SVS.FL_Secondary
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        implicitHeight: 1
                        color: Theme.borderColor
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        Layout.margins: 4
                        spacing: 2

                        ToolButton {
                            text: qsTr("Add Service")
                            display: AbstractButton.IconOnly
                            icon.source: "image://fluent-system-icons/add"
                            onClicked: page.addService()
                        }
                        ToolButton {
                            text: qsTr("Delete")
                            display: AbstractButton.IconOnly
                            icon.source: "image://fluent-system-icons/delete"
                            enabled: serviceList.currentIndex >= 0
                            onClicked: page.removeCurrentService()
                        }
                        ToolButton {
                            text: qsTr("Move Up")
                            display: AbstractButton.IconOnly
                            icon.source: "image://fluent-system-icons/arrow_up"
                            enabled: serviceList.currentIndex > 0
                            onClicked: page.moveCurrentService(-1)
                        }
                        ToolButton {
                            text: qsTr("Move Down")
                            display: AbstractButton.IconOnly
                            icon.source: "image://fluent-system-icons/arrow_down"
                            enabled: serviceList.currentIndex >= 0
                                     && serviceList.currentIndex + 1 < serviceList.count
                            onClicked: page.moveCurrentService(1)
                        }
                        Item { Layout.fillWidth: true }
                    }
                }
            }

            Item {
                SplitView.fillWidth: true
                SplitView.fillHeight: true

                ScrollView {
                    anchors.fill: parent
                    visible: Boolean(page.currentService)
                    contentWidth: availableWidth

                    ColumnLayout {
                        width: parent.width
                        spacing: 12

                        GroupBox {
                            Layout.fillWidth: true
                            Layout.leftMargin: 12
                            Layout.rightMargin: 12
                            title: qsTr("Connection")
                            TextMatcherItem on title {
                                matcher: page.matcher
                            }

                            GridLayout {
                                anchors.fill: parent
                                columns: 2
                                columnSpacing: 12
                                rowSpacing: 8

                                CheckBox {
                                    Layout.columnSpan: 2
                                    text: qsTr("Enabled")
                                    checked: page.currentService?.enabled ?? false
                                    onClicked: page.currentService.enabled = checked
                                    TextMatcherItem on text { matcher: page.matcher }
                                }
                                Label {
                                    text: qsTr("Service name")
                                    TextMatcherItem on text { matcher: page.matcher }
                                }
                                TextField {
                                    Layout.fillWidth: true
                                    text: page.currentService?.name ?? ""
                                    onTextEdited: page.currentService.name = text
                                }
                                Label {
                                    text: qsTr("Host")
                                    TextMatcherItem on text { matcher: page.matcher }
                                }
                                TextField {
                                    Layout.fillWidth: true
                                    text: page.currentService?.host ?? ""
                                    placeholderText: qsTr("localhost")
                                    onTextEdited: page.currentService.host = text
                                }
                                Label {
                                    text: qsTr("Port")
                                    TextMatcherItem on text { matcher: page.matcher }
                                }
                                SpinBox {
                                    Layout.fillWidth: true
                                    from: 1
                                    to: 65535
                                    value: page.currentService?.port ?? 80
                                    editable: true
                                    onValueModified: page.currentService.port = value
                                }
                                Label {
                                    id: endpointPathPrefixLabel
                                    readonly property string description: qsTr("Optional path prepended to every DSSP API endpoint.")
                                    text: qsTr("Endpoint path prefix")
                                    DescriptiveText.toolTip: description
                                    DescriptiveText.activated: endpointPathPrefixHoverHandler.hovered
                                    HoverHandler { id: endpointPathPrefixHoverHandler }
                                    TextMatcherItem on text { matcher: page.matcher }
                                }
                                TextField {
                                    Layout.fillWidth: true
                                    text: page.currentService?.endpointPrefix ?? ""
                                    placeholderText: qsTr("Optional")
                                    onTextEdited: page.currentService.endpointPrefix = text
                                    Accessible.description: endpointPathPrefixLabel.description
                                }
                                CheckBox {
                                    Layout.columnSpan: 2
                                    text: qsTr("Use HTTPS")
                                    checked: page.currentService?.useSsl ?? false
                                    onClicked: page.currentService.useSsl = checked
                                    TextMatcherItem on text { matcher: page.matcher }
                                }
                            }
                        }

                        GroupBox {
                            Layout.fillWidth: true
                            Layout.leftMargin: 12
                            Layout.rightMargin: 12
                            title: qsTr("Authentication")
                            TextMatcherItem on title {
                                matcher: page.matcher
                            }

                            GridLayout {
                                anchors.fill: parent
                                columns: 2
                                columnSpacing: 12
                                rowSpacing: 8

                                CheckBox {
                                    Layout.columnSpan: 2
                                    text: qsTr("Use authentication")
                                    checked: page.currentService?.authenticationEnabled ?? false
                                    onClicked: page.currentService.authenticationEnabled = checked
                                    TextMatcherItem on text { matcher: page.matcher }
                                }
                                Label {
                                    text: qsTr("API key")
                                    TextMatcherItem on text { matcher: page.matcher }
                                }
                                TextField {
                                    Layout.fillWidth: true
                                    enabled: page.currentService?.authenticationEnabled ?? false
                                    echoMode: TextInput.Password
                                    text: page.currentService?.apiKey ?? ""
                                    onTextEdited: page.currentService.apiKey = text
                                }
                            }
                        }

                        Button {
                            Layout.leftMargin: 12
                            text: page.advancedVisible
                                  ? qsTr("Hide Advanced Options")
                                  : qsTr("Show Advanced Options")
                            flat: true
                            icon.source: page.advancedVisible
                                         ? "image://fluent-system-icons/chevron_up"
                                         : "image://fluent-system-icons/chevron_down"
                            onClicked: page.advancedVisible = !page.advancedVisible
                        }

                        GroupBox {
                            visible: page.advancedVisible
                            Layout.fillWidth: true
                            Layout.leftMargin: 12
                            Layout.rightMargin: 12
                            title: qsTr("Advanced Options")
                            TextMatcherItem on title {
                                matcher: page.matcher
                            }

                            GridLayout {
                                anchors.fill: parent
                                columns: 2
                                columnSpacing: 12
                                rowSpacing: 8

                                Label {
                                    id: requestTimeoutLabel
                                    readonly property string description: qsTr("Maximum time to wait for a service request.")
                                    text: qsTr("Request timeout")
                                    DescriptiveText.toolTip: description
                                    DescriptiveText.activated: requestTimeoutHoverHandler.hovered
                                    HoverHandler { id: requestTimeoutHoverHandler }
                                    TextMatcherItem on text { matcher: page.matcher }
                                }
                                RowLayout {
                                    Layout.fillWidth: true
                                    SpinBox {
                                        Layout.fillWidth: true
                                        from: 1
                                        to: 86400
                                        value: page.currentService?.requestTimeoutSeconds ?? 30
                                        editable: true
                                        onValueModified: page.currentService.requestTimeoutSeconds = value
                                        Accessible.description: requestTimeoutLabel.description
                                    }
                                    Label { text: qsTr("seconds") }
                                }
                                Label {
                                    id: maximumRetriesLabel
                                    readonly property string description: qsTr("Number of times to retry a failed request after the initial attempt.")
                                    text: qsTr("Maximum retries")
                                    DescriptiveText.toolTip: description
                                    DescriptiveText.activated: maximumRetriesHoverHandler.hovered
                                    HoverHandler { id: maximumRetriesHoverHandler }
                                    TextMatcherItem on text { matcher: page.matcher }
                                }
                                SpinBox {
                                    Layout.fillWidth: true
                                    from: 0
                                    to: 100
                                    value: page.currentService?.retryCount ?? 5
                                    editable: true
                                    onValueModified: page.currentService.retryCount = value
                                    Accessible.description: maximumRetriesLabel.description
                                }
                                Label {
                                    id: concurrentTasksPerTypeLabel
                                    readonly property string description: qsTr("Maximum number of concurrent tasks of the same synthesis type for this service.")
                                    text: qsTr("Concurrent tasks per type")
                                    DescriptiveText.toolTip: description
                                    DescriptiveText.activated: concurrentTasksPerTypeHoverHandler.hovered
                                    HoverHandler { id: concurrentTasksPerTypeHoverHandler }
                                    TextMatcherItem on text { matcher: page.matcher }
                                }
                                SpinBox {
                                    Layout.fillWidth: true
                                    from: 1
                                    to: 65535
                                    value: page.currentService?.taskConcurrency ?? 4
                                    editable: true
                                    onValueModified: page.currentService.taskConcurrency = value
                                    Accessible.description: concurrentTasksPerTypeLabel.description
                                }
                                Label {
                                    id: totalConcurrentTasksLabel
                                    readonly property string description: qsTr("Maximum number of concurrent synthesis tasks for this service.")
                                    text: qsTr("Total concurrent tasks")
                                    DescriptiveText.toolTip: description
                                    DescriptiveText.activated: totalConcurrentTasksHoverHandler.hovered
                                    HoverHandler { id: totalConcurrentTasksHoverHandler }
                                    TextMatcherItem on text { matcher: page.matcher }
                                }
                                SpinBox {
                                    Layout.fillWidth: true
                                    from: 1
                                    to: 65535
                                    value: page.currentService?.globalConcurrency ?? 64
                                    editable: true
                                    onValueModified: page.currentService.globalConcurrency = value
                                    Accessible.description: totalConcurrentTasksLabel.description
                                }
                                Label {
                                    id: healthCheckIntervalLabel
                                    readonly property string description: qsTr("How often to check whether this service is available.")
                                    text: qsTr("Health check interval")
                                    DescriptiveText.toolTip: description
                                    DescriptiveText.activated: healthCheckIntervalHoverHandler.hovered
                                    HoverHandler { id: healthCheckIntervalHoverHandler }
                                    TextMatcherItem on text { matcher: page.matcher }
                                }
                                RowLayout {
                                    Layout.fillWidth: true
                                    SpinBox {
                                        Layout.fillWidth: true
                                        from: 1
                                        to: 86400
                                        value: page.currentService?.healthCheckIntervalSeconds ?? 60
                                        editable: true
                                        onValueModified: page.currentService.healthCheckIntervalSeconds = value
                                        Accessible.description: healthCheckIntervalLabel.description
                                    }
                                    Label { text: qsTr("seconds") }
                                }
                                CheckBox {
                                    Layout.columnSpan: 2
                                    readonly property string description: qsTr("Reject HTTPS connections with invalid or untrusted certificates.")
                                    text: qsTr("Verify TLS certificate")
                                    checked: page.currentService?.verifySslCertificate ?? true
                                    enabled: page.currentService?.useSsl ?? false
                                    onClicked: page.currentService.verifySslCertificate = checked
                                    DescriptiveText.toolTip: description
                                    DescriptiveText.activated: hovered
                                    Accessible.description: description
                                    TextMatcherItem on text { matcher: page.matcher }
                                }
                                Label {
                                    id: customRequestHeadersLabel
                                    readonly property string description: qsTr("Headers sent with every request to this service.")
                                    Layout.columnSpan: 2
                                    text: qsTr("Custom request headers")
                                    DescriptiveText.toolTip: description
                                    DescriptiveText.activated: customRequestHeadersHoverHandler.hovered
                                    HoverHandler { id: customRequestHeadersHoverHandler }
                                    TextMatcherItem on text { matcher: page.matcher }
                                }
                                ScrollView {
                                    Layout.columnSpan: 2
                                    Layout.fillWidth: true
                                    implicitHeight: 120
                                    TextArea {
                                        text: page.currentService?.customHeaders ?? ""
                                        placeholderText: qsTr("One header per line, for example:\nX-Custom-Header-1: foo\nX-Custom-Header-2: bar")
                                        wrapMode: TextEdit.NoWrap
                                        onTextChanged: if (activeFocus)
                                            page.currentService.customHeaders = text
                                        Accessible.description: customRequestHeadersLabel.description
                                    }
                                }
                            }
                        }

                        Item { Layout.fillHeight: true }
                    }
                }

                Label {
                    anchors.centerIn: parent
                    visible: !page.currentService
                    text: qsTr("Select a service to edit.")
                    ThemedItem.foregroundLevel: SVS.FL_Secondary
                }
            }
        }
    }
}
