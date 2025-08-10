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
    property bool useCustomFont
    property string fontFamily
    property string fontStyle
    property int uiBehavior
    property int graphicsBehavior
    property bool animationEnabled
    property real animationSpeedRatio

    onUseCustomFontChanged: if (started) pageHandle.markDirty()
    onFontFamilyChanged: if (started) pageHandle.markDirty()
    onFontStyleChanged: if (started) pageHandle.markDirty()
    onUiBehaviorChanged: if (started) pageHandle.markDirty()
    onGraphicsBehaviorChanged: if (started) pageHandle.markDirty()
    onAnimationEnabledChanged: if (started) pageHandle.markDirty()
    onAnimationSpeedRatioChanged: if (started) pageHandle.markDirty()

    anchors.fill: parent

    readonly property TextMatcher matcher: TextMatcher {}

    ColumnLayout {
        width: page.width
        ColumnLayout {
            Layout.fillWidth: true
            Layout.margins: 12
            spacing: 32
            GroupBox {
                title: qsTr("Font")
                TextMatcherItem on title { matcher: page.matcher }
                Layout.fillWidth: true
                ColumnLayout {
                    anchors.fill: parent
                    RowLayout {
                        Layout.fillWidth: true
                        CheckBox {
                            text: qsTr("Use custom font")
                            TextMatcherItem on text { matcher: page.matcher }
                            checked: page.useCustomFont
                            onClicked: page.useCustomFont = checked
                        }
                        ComboBox {
                            enabled: page.useCustomFont
                            readonly property list<string> families: page.pageHandle.fontFamilies()
                            model: families
                            currentIndex: families.indexOf(page.fontFamily)
                            onCurrentTextChanged: page.fontFamily = currentText
                            Layout.fillWidth: true
                        }
                        ComboBox {
                            enabled: page.useCustomFont
                            readonly property list<string> styles: page.pageHandle.fontStyles(page.fontFamily)
                            model: styles
                            currentIndex: styles.indexOf(page.fontStyle)
                            onCurrentTextChanged: page.fontStyle = currentText
                        }
                    }
                    Label {
                        ThemedItem.foregroundLevel: SVS.FL_Secondary
                        Layout.fillWidth: true
                        wrapMode: Text.Wrap
                        text: qsTr("You may need to restart %1 for font changes to take full effect.").replace("%1", Application.name)
                    }
                }
            }
            GroupBox {
                title: qsTr("User Interface")
                TextMatcherItem on title { matcher: page.matcher }
                Layout.fillWidth: true
                ColumnLayout {
                    anchors.fill: parent
                    CheckBox {
                        text: qsTr("Enable frameless window")
                        TextMatcherItem on text { matcher: page.matcher }
                        checked: page.uiBehavior & BehaviorPreference.UB_Frameless
                        onClicked: {
                            if (checked) {
                                page.uiBehavior |= BehaviorPreference.UB_Frameless
                            } else {
                                page.uiBehavior &= ~BehaviorPreference.UB_Frameless
                            }
                        }
                    }
                    Label {
                        ThemedItem.foregroundLevel: SVS.FL_Secondary
                        Layout.fillWidth: true
                        wrapMode: Text.Wrap
                        text: qsTr("Windows where frameless has been enabled need to be reopened to disable frameless.")
                    }
                    CheckBox {
                        Layout.leftMargin: 22
                        text: qsTr("Merge menu bar and title bar")
                        TextMatcherItem on text { matcher: page.matcher }
                        enabled: page.uiBehavior & BehaviorPreference.UB_Frameless
                        checked: page.uiBehavior & BehaviorPreference.UB_MergeMenuAndTitleBar
                        onClicked: {
                            if (checked) {
                                page.uiBehavior |= BehaviorPreference.UB_MergeMenuAndTitleBar
                            } else {
                                page.uiBehavior &= ~BehaviorPreference.UB_MergeMenuAndTitleBar
                            }
                        }
                    }
                    RowLayout {
                        visible: Qt.platform.os !== "windows"
                        CheckBox {
                            text: qsTr("Use native menu bar")
                            TextMatcherItem on text {
                                matcher: page.matcher
                                enabled: Qt.platform.os !== "windows"
                            }
                            checked: page.uiBehavior & BehaviorPreference.UB_NativeMenu
                            onClicked: {
                                if (checked) {
                                    page.uiBehavior |= BehaviorPreference.UB_NativeMenu
                                } else {
                                    page.uiBehavior &= ~BehaviorPreference.UB_NativeMenu
                                }
                            }
                        }
                        Label {
                            ThemedItem.foregroundLevel: SVS.FL_Secondary
                            text: qsTr("(Restart required)")
                        }
                    }
                    CheckBox {
                        text: qsTr("Show full path in the title bar of project window")
                        TextMatcherItem on text { matcher: page.matcher }
                        checked: page.uiBehavior & BehaviorPreference.UB_FullPath
                        onClicked: {
                            if (checked) {
                                page.uiBehavior |= BehaviorPreference.UB_FullPath
                            } else {
                                page.uiBehavior &= ~BehaviorPreference.UB_FullPath
                            }
                        }
                    }
                }
            }
            GroupBox {
                title: qsTr("Graphics")
                TextMatcherItem on title { matcher: page.matcher }
                Layout.fillWidth: true
                ColumnLayout {
                    anchors.fill: parent
                    RowLayout {
                        CheckBox {
                            text: qsTr("Enable hardware acceleration")
                            TextMatcherItem on text { matcher: page.matcher }
                            checked: page.graphicsBehavior & BehaviorPreference.GB_Hardware
                            onClicked: {
                                if (checked) {
                                    page.graphicsBehavior |= BehaviorPreference.GB_Hardware
                                } else {
                                    page.graphicsBehavior &= ~BehaviorPreference.GB_Hardware
                                }
                            }
                        }
                        Label {
                            ThemedItem.foregroundLevel: SVS.FL_Secondary
                            text: qsTr("(Restart required)")
                        }
                    }
                    RowLayout {
                        CheckBox {
                            text: qsTr("Enable antialiasing")
                            TextMatcherItem on text { matcher: page.matcher }
                            checked: page.graphicsBehavior & BehaviorPreference.GB_Antialiasing
                            onClicked: {
                                if (checked) {
                                    page.graphicsBehavior |= BehaviorPreference.GB_Antialiasing
                                } else {
                                    page.graphicsBehavior &= ~BehaviorPreference.GB_Antialiasing
                                }
                            }
                        }
                        Label {
                            ThemedItem.foregroundLevel: SVS.FL_Secondary
                            text: qsTr("(Restart required)")
                        }
                    }
                }
            }
            GroupBox {
                title: qsTr("Animation")
                TextMatcherItem on title { matcher: page.matcher }
                Layout.fillWidth: true
                ColumnLayout {
                    anchors.fill: parent
                    CheckBox {
                        text: qsTr("Enable animation")
                        TextMatcherItem on text { matcher: page.matcher }
                        checked: page.animationEnabled
                        onClicked: page.animationEnabled = checked
                    }
                    RowLayout {
                        enabled: page.animationEnabled
                        Label {
                            text: qsTr("Animation speed ratio")
                            TextMatcherItem on text { matcher: page.matcher }
                        }
                        SpinBox {
                            from: 10
                            to: 1000
                            value: page.animationSpeedRatio * 100
                            onValueModified: page.animationSpeedRatio = value / 100.0
                            textFromValue: function(value, locale) {
                                return value + "%"
                            }
                            valueFromText: function(text, locale) {
                                return parseInt(text.replace("%", ""))
                            }
                        }
                    }
                }
            }
        }
    }
}