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
    property int uiBehavior
    property int graphicsBehavior
    property bool animationEnabled
    property real animationSpeedRatio

    onUseCustomFontChanged: if (started) pageHandle.markDirty()
    onFontFamilyChanged: if (started) pageHandle.markDirty()
    onUiBehaviorChanged: if (started) pageHandle.markDirty()
    onGraphicsBehaviorChanged: if (started) pageHandle.markDirty()
    onAnimationEnabledChanged: if (started) pageHandle.markDirty()
    onAnimationSpeedRatioChanged: if (started) pageHandle.markDirty()

    anchors.fill: parent
    ColumnLayout {
        width: page.width
        ColumnLayout {
            Layout.fillWidth: true
            Layout.margins: 12
            spacing: 32
            GroupBox {
                title: qsTr("Font")
                Layout.fillWidth: true
                ColumnLayout {
                    anchors.fill: parent
                    RowLayout {
                        Layout.fillWidth: true
                        CheckBox {
                            text: qsTr("Use custom font")
                            checked: page.useCustomFont
                            onClicked: page.useCustomFont = checked
                        }
                        ComboBox {
                            enabled: page.useCustomFont
                            Layout.fillWidth: true
                        }
                    }
                }
            }
            GroupBox {
                title: qsTr("User Interface")
                Layout.fillWidth: true
                ColumnLayout {
                    anchors.fill: parent
                    RowLayout {
                        CheckBox {
                            text: qsTr("Enable frameless window")
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
                            text: qsTr("(Restart required)")
                        }
                    }
                    CheckBox {
                        Layout.leftMargin: 22
                        text: qsTr("Merge menu bar and title bar")
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
                        CheckBox {
                            text: qsTr("Use native menu bar")
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
                Layout.fillWidth: true
                ColumnLayout {
                    anchors.fill: parent
                    RowLayout {
                        CheckBox {
                            text: qsTr("Enable hardware acceleration")
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
                Layout.fillWidth: true
                ColumnLayout {
                    anchors.fill: parent
                    CheckBox {
                        text: qsTr("Enable animation")
                        checked: page.animationEnabled
                        onClicked: page.animationEnabled = checked
                    }
                    RowLayout {
                        enabled: page.animationEnabled
                        Label {
                            text: qsTr("Animation speed ratio")
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