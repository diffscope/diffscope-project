// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

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
    property int startupBehavior
    property bool useSystemLanguage
    property string localeName
    property string fallbackLyricLanguageCode
    property string fallbackLyricText
    property bool hasNotificationSoundAlert
    property int notificationAutoHideTimeout
    property int commandPaletteHistoryCount
    property int proxyOption
    property int proxyType
    property string proxyHostname
    property int proxyPort
    property bool proxyHasAuthentication
    property string proxyUsername
    property string proxyPassword
    property bool shouldStoreGeometry
    readonly property string selectedLocaleName: useSystemLanguage ? pageHandle.systemLocaleName : localeName

    function languageDisplayName(localeName) {
        let locale = Qt.locale(localeName)
        return `${locale.nativeLanguageName} (${locale.nativeTerritoryName})`
    }

    onStartupBehaviorChanged: if (started) pageHandle.markDirty()
    onUseSystemLanguageChanged: if (started) pageHandle.markDirty()
    onLocaleNameChanged: if (started) pageHandle.markDirty()
    onFallbackLyricLanguageCodeChanged: if (started) pageHandle.markDirty()
    onFallbackLyricTextChanged: if (started) pageHandle.markDirty()
    onHasNotificationSoundAlertChanged: if (started) pageHandle.markDirty()
    onNotificationAutoHideTimeoutChanged: if (started) pageHandle.markDirty()
    onProxyOptionChanged: if (started) pageHandle.markDirty()
    onProxyTypeChanged: if (started) pageHandle.markDirty()
    onProxyHostnameChanged: if (started) pageHandle.markDirty()
    onProxyPortChanged: if (started) pageHandle.markDirty()
    onProxyHasAuthenticationChanged: if (started) pageHandle.markDirty()
    onProxyUsernameChanged: if (started) pageHandle.markDirty()
    onProxyPasswordChanged: if (started) pageHandle.markDirty()
    onShouldStoreGeometryChanged: if (started) pageHandle.markDirty()

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
                title: qsTr("Startup")
                TextMatcherItem on title { matcher: page.matcher }
                Layout.fillWidth: true
                ColumnLayout {
                    anchors.fill: parent
                    RowLayout {
                        spacing: 16
                        Label {
                            text: qsTr("When starting %1").arg(Application.displayName)
                            TextMatcherItem on text { matcher: page.matcher }
                        }
                        RadioButton {
                            text: qsTr("Open the home window")
                            TextMatcherItem on text { matcher: page.matcher }
                            checked: !(page.startupBehavior & BehaviorPreference.SB_CreateNewProject)
                            onClicked: () => {
                                if (checked) {
                                    page.startupBehavior &= ~BehaviorPreference.SB_CreateNewProject
                                } else {
                                    page.startupBehavior |= BehaviorPreference.SB_CreateNewProject
                                }
                            }

                        }
                        RadioButton {
                            text: qsTr("Create a new project")
                            TextMatcherItem on text { matcher: page.matcher }
                            checked: page.startupBehavior & BehaviorPreference.SB_CreateNewProject
                            onClicked: () => {
                                if (checked) {
                                    page.startupBehavior |= BehaviorPreference.SB_CreateNewProject
                                } else {
                                    page.startupBehavior &= ~BehaviorPreference.SB_CreateNewProject
                                }
                            }
                        }
                    }
                    CheckBox {
                        text: qsTr("Close the home window after opening a project")
                        TextMatcherItem on text { matcher: page.matcher }
                        checked: page.startupBehavior & BehaviorPreference.SB_CloseHomeWindowAfterOpeningProject
                        onClicked: () => {
                            if (checked) {
                                page.startupBehavior |= BehaviorPreference.SB_CloseHomeWindowAfterOpeningProject
                            } else {
                                page.startupBehavior &= ~BehaviorPreference.SB_CloseHomeWindowAfterOpeningProject
                            }
                        }
                    }
                    CheckBox {
                        Layout.leftMargin: 22
                        text: qsTr("Reopen the home window when the last project window closes")
                        TextMatcherItem on text { matcher: page.matcher }
                        checked: page.startupBehavior & BehaviorPreference.SB_OpenHomeWindowWhenLastProjectClosed
                        enabled: page.startupBehavior & BehaviorPreference.SB_CloseHomeWindowAfterOpeningProject
                        onClicked: () => {
                            if (checked) {
                                page.startupBehavior |= BehaviorPreference.SB_OpenHomeWindowWhenLastProjectClosed
                            } else {
                                page.startupBehavior &= ~BehaviorPreference.SB_OpenHomeWindowWhenLastProjectClosed
                            }
                        }
                    }
                }
            }
            GroupBox {
                title: qsTr("Language")
                TextMatcherItem on title { matcher: page.matcher }
                Layout.fillWidth: true
                ColumnLayout {
                    anchors.fill: parent
                    CheckBox {
                        text: qsTr("Use system language")
                        TextMatcherItem on text { matcher: page.matcher }
                        checked: page.useSystemLanguage
                        onClicked: () => {
                            page.useSystemLanguage = checked
                            if (!checked && languageComboBox.indexOfValue(page.localeName) === -1
                                    && languageComboBox.count > 0) {
                                page.localeName = languageComboBox.valueAt(0)
                            }
                        }
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        Label {
                            text: qsTr("Language")
                            property string languageText: "Language"
                            TextMatcherItem on text { matcher: page.matcher }
                            TextMatcherItem on languageText { matcher: page.matcher }
                        }
                        ComboBox {
                            id: languageComboBox
                            Layout.fillWidth: true
                            model: page.pageHandle.languages
                            textRole: "text"
                            valueRole: "value"
                            enabled: !page.useSystemLanguage
                            Component.onCompleted: () => {
                                currentIndex = Qt.binding(() => indexOfValue(page.selectedLocaleName))
                                displayText = Qt.binding(() => {
                                    let index = indexOfValue(page.selectedLocaleName)
                                    if (index === -1) {
                                        return page.languageDisplayName(page.selectedLocaleName)
                                    } else {
                                        return currentText
                                    }
                                })
                            }
                            onActivated: (index) => page.localeName = valueAt(index)
                        }
                        Label {
                            ThemedItem.foregroundLevel: SVS.FL_Secondary
                            text: qsTr("(Restart required)")
                            visible: page.selectedLocaleName !== page.pageHandle.currentLocaleName
                        }
                    }
                    GridLayout {
                        Layout.fillWidth: true
                        columns: 2
                        Label {
                            text: qsTr("Fallback lyric language code")
                            TextMatcherItem on text { matcher: page.matcher }
                        }
                        TextField {
                            Layout.fillWidth: true
                            text: page.fallbackLyricLanguageCode
                            onTextEdited: page.fallbackLyricLanguageCode = text
                        }
                        Label {
                            text: qsTr("Fallback lyric text")
                            TextMatcherItem on text { matcher: page.matcher }
                        }
                        TextField {
                            Layout.fillWidth: true
                            text: page.fallbackLyricText
                            onTextEdited: page.fallbackLyricText = text
                        }
                    }
                }
            }
            GroupBox {
                title: qsTr("Notification")
                TextMatcherItem on title { matcher: page.matcher }
                Layout.fillWidth: true
                ColumnLayout {
                    anchors.fill: parent
                    CheckBox {
                        text: qsTr("Play sound alert when a notification bubble is sent")
                        TextMatcherItem on text { matcher: page.matcher }
                        checked: page.hasNotificationSoundAlert
                        onClicked: page.hasNotificationSoundAlert = checked
                    }
                    RowLayout {
                        Label {
                            text: qsTr("Timeout for auto hiding notification bubbles")
                            TextMatcherItem on text { matcher: page.matcher }
                        }
                        SpinBox {
                            from: 0
                            to: 2147483647
                            value: page.notificationAutoHideTimeout
                            onValueModified: page.notificationAutoHideTimeout = value
                        }
                        Label {
                            text: qsTr("milliseconds")
                        }
                    }
                    Button {
                        text: qsTr('Reset All "Do Not Show Again"')
                        TextMatcherItem on text { matcher: page.matcher }
                        onClicked: CoreInterface.resetAllDoNotShowAgainRequested()
                    }
                }
            }
            GroupBox {
                title: qsTr("Window")
                TextMatcherItem on title { matcher: page.matcher }
                Layout.fillWidth: true
                ColumnLayout {
                    anchors.fill: parent
                    CheckBox {
                        text: qsTr("Memorize window position and size")
                        TextMatcherItem on text { matcher: page.matcher }
                        checked: page.shouldStoreGeometry
                        onClicked: page.shouldStoreGeometry = checked
                    }
                }
            }
            GroupBox {
                title: qsTr("Find Actions")
                TextMatcherItem on title { matcher: page.matcher }
                Layout.fillWidth: true
                ColumnLayout {
                    anchors.fill: parent
                    RowLayout {
                        Label {
                            text: qsTr('Number of "recently used" records')
                            TextMatcherItem on text { matcher: page.matcher }
                        }
                        SpinBox {
                            from: 0
                            to: 2147483647
                            value: page.commandPaletteHistoryCount
                            onValueModified: page.commandPaletteHistoryCount = value
                        }
                    }
                    RowLayout {
                        Button {
                            text: qsTr("Clear History")
                            TextMatcherItem on text { matcher: page.matcher }
                            onClicked: () => {
                                BehaviorPreference.commandPaletteClearHistoryRequested()
                                historyClearedLabel.visible = true
                            }
                        }
                        Label {
                            id: historyClearedLabel
                            text: qsTr("History cleared")
                            visible: false
                            ThemedItem.foregroundLevel: SVS.FL_Secondary
                            Connections {
                                target: page
                                function onStartedChanged() {
                                    historyClearedLabel.visible = false
                                }
                            }
                        }
                    }
                }
            }
            GroupBox {
                title: qsTr("Network Proxy")
                TextMatcherItem on title { matcher: page.matcher }
                Layout.fillWidth: true
                ColumnLayout {
                    anchors.fill: parent
                    RadioButton {
                        text: qsTr("No proxy")
                        TextMatcherItem on text { matcher: page.matcher }
                        checked: page.proxyOption === BehaviorPreference.PO_None
                        onClicked: page.proxyOption = BehaviorPreference.PO_None
                    }
                    RadioButton {
                        text: qsTr("Use system proxy")
                        TextMatcherItem on text { matcher: page.matcher }
                        checked: page.proxyOption === BehaviorPreference.PO_System
                        onClicked: page.proxyOption = BehaviorPreference.PO_System
                    }
                    RadioButton {
                        text: qsTr("Manually configure proxy")
                        TextMatcherItem on text { matcher: page.matcher }
                        checked: page.proxyOption === BehaviorPreference.PO_Manual
                        onClicked: page.proxyOption = BehaviorPreference.PO_Manual
                    }
                    ColumnLayout {
                        Layout.fillWidth: true
                        Layout.leftMargin: 22
                        enabled: page.proxyOption === BehaviorPreference.PO_Manual
                        GridLayout {
                            Layout.fillWidth: true
                            columns: 2
                            Label {
                                text: qsTr("Type")
                                TextMatcherItem on text { matcher: page.matcher }
                            }
                            ComboBox {
                                Layout.fillWidth: true
                                textRole: "text"
                                valueRole: "value"
                                model: [
                                    { text: qsTr("SOCK5"), value: BehaviorPreference.PT_SOCKS5 },
                                    { text: qsTr("HTTP"), value: BehaviorPreference.PT_HTTP },
                                ]
                                currentIndex: page.proxyType
                                onCurrentValueChanged: page.proxyType = currentValue
                            }
                            Label {
                                text: qsTr("Hostname")
                                TextMatcherItem on text { matcher: page.matcher }
                            }
                            TextField {
                                Layout.fillWidth: true
                                text: page.proxyHostname
                                onTextEdited: page.proxyHostname = text
                            }
                            Label {
                                text: qsTr("Port")
                                TextMatcherItem on text { matcher: page.matcher }
                            }
                            SpinBox {
                                Layout.fillWidth: true
                                from: 0
                                to: 65535
                                value: page.proxyPort
                                onValueModified: page.proxyPort = value
                            }
                        }
                        CheckBox {
                            text: qsTr("Authentication")
                            TextMatcherItem on text { matcher: page.matcher }
                            checked: page.proxyHasAuthentication
                            onClicked: page.proxyHasAuthentication = checked
                        }
                        GridLayout {
                            Layout.fillWidth: true
                            Layout.leftMargin: 22
                            columns: 2
                            enabled: page.proxyHasAuthentication
                            Label {
                                text: qsTr("Username")
                                TextMatcherItem on text { matcher: page.matcher }
                            }
                            TextField {
                                Layout.fillWidth: true
                                text: page.proxyUsername
                                onTextEdited: page.proxyUsername = text
                            }
                            Label {
                                text: qsTr("Password")
                                TextMatcherItem on text { matcher: page.matcher }
                            }
                            TextField {
                                Layout.fillWidth: true
                                echoMode: TextInput.Password
                                text: page.proxyPassword
                                onTextEdited: page.proxyPassword = text
                            }
                        }
                    }
                }
            }
        }
    }
}
