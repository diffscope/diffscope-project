// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

import QtQml
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

import SVSCraft
import SVSCraft.UIComponents
import SVSCraft.UIComponents.impl

import DiffScope.UIShell

WelcomeWizardPage {
    id: page
    title: qsTr("Color Scheme")
    description: qsTr("Choose a color scheme for %1").arg(Application.displayName)
    iconSource: "image://fluent-system-icons/color?style=regular"

    onAboutToEnter: helper.reload()

    ColorSchemeWelcomeWizardPageHelper {
        id: helper
    }

    ColumnLayout {
        anchors.centerIn: parent
        spacing: 24
        GridLayout {
            rowSpacing: 8
            columnSpacing: 64
            columns: 3
            Layout.alignment: Qt.AlignHCenter
            RadioButton {
                id: darkRadioButton
                text: qsTr("Dark")
                checked: helper.currentPresetIndex === 0
                onClicked: helper.applyInternalPreset(0)
            }
            RadioButton {
                id: lightRadioButton
                text: qsTr("Light")
                checked: helper.currentPresetIndex === 1
                onClicked: helper.applyInternalPreset(1)
            }
            RadioButton {
                id: highContrastRadioButton
                text: qsTr("High contrast")
                checked: helper.currentPresetIndex === 2
                onClicked: helper.applyInternalPreset(2)
            }
            ThemePreviewThumbnail {
                readonly property var d: helper.previewPresets[0]
                accentColor: d.accentColor
                buttonColor: d.buttonColor
                textFieldColor: d.textFieldColor
                borderColor: d.borderColor
                backgroundPrimaryColor: d.backgroundPrimaryColor
                backgroundSecondaryColor: d.backgroundSecondaryColor
                backgroundTertiaryColor: d.backgroundTertiaryColor
                backgroundQuaternaryColor: d.backgroundQuaternaryColor
                foregroundPrimaryColor: d.foregroundPrimaryColor
                foregroundSecondaryColor: d.foregroundSecondaryColor
                TapHandler {
                    onSingleTapped: darkRadioButton.click()
                }
            }
            ThemePreviewThumbnail {
                readonly property var d: helper.previewPresets[1]
                accentColor: d.accentColor
                buttonColor: d.buttonColor
                textFieldColor: d.textFieldColor
                borderColor: d.borderColor
                backgroundPrimaryColor: d.backgroundPrimaryColor
                backgroundSecondaryColor: d.backgroundSecondaryColor
                backgroundTertiaryColor: d.backgroundTertiaryColor
                backgroundQuaternaryColor: d.backgroundQuaternaryColor
                foregroundPrimaryColor: d.foregroundPrimaryColor
                foregroundSecondaryColor: d.foregroundSecondaryColor
                TapHandler {
                    onSingleTapped: lightRadioButton.click()
                }
            }
            ThemePreviewThumbnail {
                readonly property var d: helper.previewPresets[2]
                accentColor: d.accentColor
                buttonColor: d.buttonColor
                textFieldColor: d.textFieldColor
                borderColor: d.borderColor
                backgroundPrimaryColor: d.backgroundPrimaryColor
                backgroundSecondaryColor: d.backgroundSecondaryColor
                backgroundTertiaryColor: d.backgroundTertiaryColor
                backgroundQuaternaryColor: d.backgroundQuaternaryColor
                foregroundPrimaryColor: d.foregroundPrimaryColor
                foregroundSecondaryColor: d.foregroundSecondaryColor
                TapHandler {
                    onSingleTapped: highContrastRadioButton.click()
                }
            }
        }
        RowLayout {
            Layout.alignment: Qt.AlignHCenter
            visible: helper.currentPresetIndex >= 0
            spacing: 8
            Repeater {
                model: helper.accentColors
                delegate: Button {
                    id: colorButton
                    required property color modelData
                    required property int index
                    Accessible.name: qsTr("Accent Color %L1").arg(index + 1)
                    display: AbstractButton.IconOnly
                    padding: 4
                    horizontalPadding: 4
                    flat: !checked
                    checkable: true
                    autoExclusive: true
                    checked: helper.currentAccentColorIndex === index
                    onClicked: helper.applyAccentColor(index)
                    contentItem: Rectangle {
                        implicitWidth: 32
                        implicitHeight: 32
                        color: colorButton.enabled ? colorButton.modelData : Theme.controlDisabledColorChange.apply(colorButton.modelData)
                        border.color: colorButton.checked ? Theme.foregroundPrimaryColor : Theme.borderColor
                        border.width: colorButton.checked ? 2 : 1
                        radius: width / 2
                    }
                    background: ButtonRectangle {
                        control: colorButton
                        implicitWidth: 36
                        implicitHeight: 36
                        radius: height / 2
                        down: colorButton.checked && colorButton.down
                        hovered: colorButton.checked && colorButton.hovered
                    }
                }
            }
        }
        Label {
            Layout.alignment: Qt.AlignHCenter
            text: qsTr("You can also create a custom color theme later in Settings > Appearance > Color Scheme")
        }
    }
}
