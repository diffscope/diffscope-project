import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SVSCraft
import SVSCraft.UIComponents

ScrollView {
    id: page

    required property QtObject pageHandle
    property bool started: false
    property int startupBehavior
    property bool useSystemLanguage
    property string localeName
    property bool hasNotificationSoundAlert
    property int notificationAutoHideTimeout
    property int proxyOption
    property int proxyType
    property string proxyHostname
    property int proxyPort
    property bool proxyHasAuthentication
    property string proxyUsername
    property string proxyPassword
    property bool autoCheckForUpdates
    property int updateOption

    onStartupBehaviorChanged: if (started) pageHandle.markDirty()
    onUseSystemLanguageChanged: if (started) pageHandle.markDirty()
    onLocaleNameChanged: if (started) pageHandle.markDirty()
    onHasNotificationSoundAlertChanged: if (started) pageHandle.markDirty()
    onNotificationAutoHideTimeoutChanged: if (started) pageHandle.markDirty()
    onProxyOptionChanged: if (started) pageHandle.markDirty()
    onProxyTypeChanged: if (started) pageHandle.markDirty()
    onProxyHostnameChanged: if (started) pageHandle.markDirty()
    onProxyPortChanged: if (started) pageHandle.markDirty()
    onProxyHasAuthenticationChanged: if (started) pageHandle.markDirty()
    onProxyUsernameChanged: if (started) pageHandle.markDirty()
    onProxyPasswordChanged: if (started) pageHandle.markDirty()
    onAutoCheckForUpdatesChanged: if (started) pageHandle.markDirty()
    onUpdateOptionChanged: if (started) pageHandle.markDirty()

    anchors.fill: parent
    ColumnLayout {
        width: page.width
        ColumnLayout {
            Layout.fillWidth: true
            Layout.margins: 12
            spacing: 32
            GroupBox {
                title: qsTr("Startup")
                Layout.fillWidth: true
                ColumnLayout {
                    anchors.fill: parent
                    RowLayout {
                        spacing: 16
                        Label {
                            text: qsTr("When starting %1").replace("%1", Application.name)
                        }
                        RadioButton {
                            text: qsTr("Open the home window")
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
                        text: qsTr("Open previous projects on startup automatically")
                        checked: page.startupBehavior & BehaviorPreference.SB_AutoOpenPreviousProjects
                        onClicked: () => {
                            if (checked) {
                                page.startupBehavior |= BehaviorPreference.SB_AutoOpenPreviousProjects
                            } else {
                                page.startupBehavior &= ~BehaviorPreference.SB_AutoOpenPreviousProjects
                            }
                        }
                    }
                    CheckBox {
                        text: qsTr("Close the home window after opening a project")
                        checked: page.startupBehavior & BehaviorPreference.SB_CloseHomeWindowAfterOpeningProject
                        onClicked: () => {
                            if (checked) {
                                page.startupBehavior |= BehaviorPreference.SB_CloseHomeWindowAfterOpeningProject
                            } else {
                                page.startupBehavior &= ~BehaviorPreference.SB_CloseHomeWindowAfterOpeningProject
                            }
                        }
                    }
                }
            }
            GroupBox {
                title: qsTr("Language")
                Layout.fillWidth: true
                ColumnLayout {
                    anchors.fill: parent
                    CheckBox {
                        text: qsTr("Use system language")
                        checked: page.useSystemLanguage
                        onClicked: page.useSystemLanguage = checked
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        enabled: !page.useSystemLanguage
                        Label {
                            text: qsTr("Language")
                        }
                        ComboBox {
                            Layout.fillWidth: true
                        }
                        Label {
                            ThemedItem.foregroundLevel: SVS.FL_Secondary
                            text: qsTr("(Restart required)")
                        }
                    }
                }
            }
            GroupBox {
                title: qsTr("Notification")
                Layout.fillWidth: true
                ColumnLayout {
                    anchors.fill: parent
                    CheckBox {
                        text: qsTr("Play sound alert when a notification bubble is sent")
                        checked: page.hasNotificationSoundAlert
                        onClicked: page.hasNotificationSoundAlert = checked
                    }
                    RowLayout {
                        Label {
                            text: qsTr("Timeout for auto hiding notification bubbles")
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
                    }
                }
            }
            GroupBox {
                title: qsTr("Network Proxy")
                Layout.fillWidth: true
                ColumnLayout {
                    anchors.fill: parent
                    RadioButton {
                        text: qsTr("No proxy")
                        checked: page.proxyOption === BehaviorPreference.PO_None
                        onClicked: page.proxyOption = BehaviorPreference.PO_None
                    }
                    RadioButton {
                        text: qsTr("Use system proxy")
                        checked: page.proxyOption === BehaviorPreference.PO_System
                        onClicked: page.proxyOption = BehaviorPreference.PO_System
                    }
                    RadioButton {
                        text: qsTr("Manually configure proxy")
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
                            }
                            TextField {
                                Layout.fillWidth: true
                                text: page.proxyHostname
                                onTextEdited: page.proxyHostname = text
                            }
                            Label {
                                text: qsTr("Port")
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
                            }
                            TextField {
                                Layout.fillWidth: true
                                text: page.proxyUsername
                                onTextEdited: page.proxyUsername = text
                            }
                            Label {
                                text: qsTr("Password")
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
            GroupBox {
                title: qsTr("Updates")
                Layout.fillWidth: true
                ColumnLayout {
                    Layout.fillWidth: true
                    CheckBox {
                        text: qsTr("Check for updates on startup")
                        checked: page.autoCheckForUpdates
                        onClicked: page.autoCheckForUpdates = checked
                    }
                    RowLayout {
                        Label {
                            text: qsTr("Type of update to check for")
                        }
                        ComboBox {
                            textRole: "text"
                            valueRole: "value"
                            model: [
                                { text: qsTr("Stable"), value: BehaviorPreference.UO_Stable },
                                { text: qsTr("Beta"), value: BehaviorPreference.UO_Beta },
                            ]
                            currentIndex: page.updateOption
                            onCurrentValueChanged: page.updateOption = currentValue
                        }
                    }
                    Button {
                        text: qsTr("Check for Updates")
                    }
                }
            }
        }
    }
}