// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

pragma ComponentBehavior: Bound

import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SVSCraft
import SVSCraft.UIComponents

ColumnLayout {
    id: editor

    required property var currentService
    property TextMatcher matcher: TextMatcher {}
    property bool advancedVisible: false

    spacing: 12
    onCurrentServiceChanged: advancedVisible = false

    GroupBox {
        Layout.fillWidth: true
        Layout.leftMargin: 12
        Layout.rightMargin: 12
        title: qsTr("Connection")
        TextMatcherItem on title {
            matcher: editor.matcher
        }

        GridLayout {
            anchors.fill: parent
            columns: 2
            columnSpacing: 12
            rowSpacing: 8

            CheckBox {
                Layout.columnSpan: 2
                text: qsTr("Enabled")
                checked: editor.currentService?.enabled ?? false
                onClicked: editor.currentService.enabled = checked
                TextMatcherItem on text { matcher: editor.matcher }
            }
            Label {
                id: serviceNameLabel
                text: qsTr("Service name")
                TextMatcherItem on text { matcher: editor.matcher }
            }
            TextField {
                Layout.fillWidth: true
                text: editor.currentService?.name ?? ""
                Accessible.name: serviceNameLabel.text
                onTextEdited: editor.currentService.name = text
            }
            Label {
                id: hostLabel
                text: qsTr("Host")
                TextMatcherItem on text { matcher: editor.matcher }
            }
            TextField {
                Layout.fillWidth: true
                text: editor.currentService?.host ?? ""
                placeholderText: qsTr("localhost")
                Accessible.name: hostLabel.text
                onTextEdited: editor.currentService.host = text
            }
            Label {
                id: portLabel
                text: qsTr("Port")
                TextMatcherItem on text { matcher: editor.matcher }
            }
            SpinBox {
                Layout.fillWidth: true
                from: 1
                to: 65535
                value: editor.currentService?.port ?? 80
                Accessible.name: portLabel.text
                onValueModified: editor.currentService.port = value
            }
            Label {
                id: endpointPathPrefixLabel
                readonly property string description: qsTr("Optional path prepended to every DSSP API endpoint.")
                text: qsTr("Endpoint path prefix")
                DescriptiveText.toolTip: description
                DescriptiveText.activated: endpointPathPrefixHoverHandler.hovered
                HoverHandler { id: endpointPathPrefixHoverHandler }
                TextMatcherItem on text { matcher: editor.matcher }
            }
            TextField {
                Layout.fillWidth: true
                text: editor.currentService?.endpointPrefix ?? ""
                placeholderText: qsTr("Optional")
                onTextEdited: editor.currentService.endpointPrefix = text
                Accessible.name: endpointPathPrefixLabel.text
                Accessible.description: endpointPathPrefixLabel.description
            }
            CheckBox {
                Layout.columnSpan: 2
                text: qsTr("Use HTTPS")
                checked: editor.currentService?.useSsl ?? false
                onClicked: editor.currentService.useSsl = checked
                TextMatcherItem on text { matcher: editor.matcher }
            }
        }
    }

    GroupBox {
        Layout.fillWidth: true
        Layout.leftMargin: 12
        Layout.rightMargin: 12
        title: qsTr("Authentication")
        TextMatcherItem on title {
            matcher: editor.matcher
        }

        GridLayout {
            anchors.fill: parent
            columns: 2
            columnSpacing: 12
            rowSpacing: 8

            CheckBox {
                Layout.columnSpan: 2
                text: qsTr("Use authentication")
                checked: editor.currentService?.authenticationEnabled ?? false
                onClicked: editor.currentService.authenticationEnabled = checked
                TextMatcherItem on text { matcher: editor.matcher }
            }
            Label {
                id: apiKeyLabel
                text: qsTr("API key")
                TextMatcherItem on text { matcher: editor.matcher }
            }
            TextField {
                Layout.fillWidth: true
                enabled: editor.currentService?.authenticationEnabled ?? false
                echoMode: TextInput.Password
                text: editor.currentService?.apiKey ?? ""
                Accessible.name: apiKeyLabel.text
                onTextEdited: editor.currentService.apiKey = text
            }
        }
    }

    Button {
        Layout.leftMargin: 12
        text: editor.advancedVisible
              ? qsTr("Hide Advanced Options")
              : qsTr("Show Advanced Options")
        flat: true
        icon.source: editor.advancedVisible
                     ? "image://fluent-system-icons/chevron_up"
                     : "image://fluent-system-icons/chevron_down"
        onClicked: editor.advancedVisible = !editor.advancedVisible
    }

    GroupBox {
        visible: editor.advancedVisible
        Layout.fillWidth: true
        Layout.leftMargin: 12
        Layout.rightMargin: 12
        title: qsTr("Advanced Options")
        TextMatcherItem on title {
            matcher: editor.matcher
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
                TextMatcherItem on text { matcher: editor.matcher }
            }
            RowLayout {
                Layout.fillWidth: true
                SpinBox {
                    Layout.fillWidth: true
                    from: 1
                    to: 86400
                    value: editor.currentService?.requestTimeoutSeconds ?? 30
                    onValueModified: editor.currentService.requestTimeoutSeconds = value
                    Accessible.name: requestTimeoutLabel.text
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
                TextMatcherItem on text { matcher: editor.matcher }
            }
            SpinBox {
                Layout.fillWidth: true
                from: 0
                to: 100
                value: editor.currentService?.retryCount ?? 5
                onValueModified: editor.currentService.retryCount = value
                Accessible.name: maximumRetriesLabel.text
                Accessible.description: maximumRetriesLabel.description
            }
            Label {
                id: concurrentTasksPerTypeLabel
                readonly property string description: qsTr("Maximum number of concurrent tasks of the same synthesis type for this service.")
                text: qsTr("Concurrent tasks per type")
                DescriptiveText.toolTip: description
                DescriptiveText.activated: concurrentTasksPerTypeHoverHandler.hovered
                HoverHandler { id: concurrentTasksPerTypeHoverHandler }
                TextMatcherItem on text { matcher: editor.matcher }
            }
            SpinBox {
                Layout.fillWidth: true
                from: 1
                to: 65535
                value: editor.currentService?.taskConcurrency ?? 4
                onValueModified: editor.currentService.taskConcurrency = value
                Accessible.name: concurrentTasksPerTypeLabel.text
                Accessible.description: concurrentTasksPerTypeLabel.description
            }
            Label {
                id: totalConcurrentTasksLabel
                readonly property string description: qsTr("Maximum number of concurrent synthesis tasks for this service.")
                text: qsTr("Total concurrent tasks")
                DescriptiveText.toolTip: description
                DescriptiveText.activated: totalConcurrentTasksHoverHandler.hovered
                HoverHandler { id: totalConcurrentTasksHoverHandler }
                TextMatcherItem on text { matcher: editor.matcher }
            }
            SpinBox {
                Layout.fillWidth: true
                from: 1
                to: 65535
                value: editor.currentService?.globalConcurrency ?? 64
                onValueModified: editor.currentService.globalConcurrency = value
                Accessible.name: totalConcurrentTasksLabel.text
                Accessible.description: totalConcurrentTasksLabel.description
            }
            Label {
                id: healthCheckIntervalLabel
                readonly property string description: qsTr("How often to check whether this service is available.")
                text: qsTr("Health check interval")
                DescriptiveText.toolTip: description
                DescriptiveText.activated: healthCheckIntervalHoverHandler.hovered
                HoverHandler { id: healthCheckIntervalHoverHandler }
                TextMatcherItem on text { matcher: editor.matcher }
            }
            RowLayout {
                Layout.fillWidth: true
                SpinBox {
                    Layout.fillWidth: true
                    from: 1
                    to: 86400
                    value: editor.currentService?.healthCheckIntervalSeconds ?? 60
                    onValueModified: editor.currentService.healthCheckIntervalSeconds = value
                    Accessible.name: healthCheckIntervalLabel.text
                    Accessible.description: healthCheckIntervalLabel.description
                }
                Label { text: qsTr("seconds") }
            }
            CheckBox {
                Layout.columnSpan: 2
                readonly property string description: qsTr("Reject HTTPS connections with invalid or untrusted certificates.")
                text: qsTr("Verify TLS certificate")
                checked: editor.currentService?.verifySslCertificate ?? true
                enabled: editor.currentService?.useSsl ?? false
                onClicked: editor.currentService.verifySslCertificate = checked
                DescriptiveText.toolTip: description
                Accessible.description: description
                TextMatcherItem on text { matcher: editor.matcher }
            }
            Label {
                id: customRequestHeadersLabel
                readonly property string description: qsTr("Headers sent with every request to this service.")
                Layout.columnSpan: 2
                text: qsTr("Custom request headers")
                DescriptiveText.toolTip: description
                DescriptiveText.activated: customRequestHeadersHoverHandler.hovered
                HoverHandler { id: customRequestHeadersHoverHandler }
                TextMatcherItem on text { matcher: editor.matcher }
            }
            ScrollView {
                Layout.columnSpan: 2
                Layout.fillWidth: true
                implicitHeight: 120
                TextArea {
                    text: editor.currentService?.customHeaders ?? ""
                    placeholderText: qsTr("One header per line, for example:\nX-Custom-Header-1: foo\nX-Custom-Header-2: bar")
                    wrapMode: TextEdit.NoWrap
                    onTextChanged: if (activeFocus)
                        editor.currentService.customHeaders = text
                    Accessible.name: customRequestHeadersLabel.text
                    Accessible.description: customRequestHeadersLabel.description
                }
            }
        }
    }

    Item { Layout.fillHeight: true }
}

