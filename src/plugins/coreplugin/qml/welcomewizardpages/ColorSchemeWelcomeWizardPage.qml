import QtQml
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

import SVSCraft
import SVSCraft.UIComponents

import DiffScope.UIShell

WelcomeWizardPage {
    id: page
    title: qsTr("Color Scheme")
    description: qsTr("Choose a color scheme for %1").arg(Application.name)

    ColorSchemeWelcomeWizardPageHelper {
        id: helper
    }

    ColumnLayout {
        anchors.centerIn: parent
        spacing: 64
        GridLayout {
            rowSpacing: 8
            columnSpacing: 64
            columns: 3
            Layout.alignment: Qt.AlignHCenter
            RadioButton {
                id: darkRadioButton
                text: qsTr("Dark")
                checked: helper.getCurrentPresetIndex() === 0
                onClicked: helper.applyInternalPreset(0)
            }
            RadioButton {
                id: lightRadioButton
                text: qsTr("Light")
                checked: helper.getCurrentPresetIndex() === 1
                onClicked: helper.applyInternalPreset(1)
            }
            RadioButton {
                id: highContrastRadioButton
                text: qsTr("High contrast")
                checked: helper.getCurrentPresetIndex() === 2
                onClicked: helper.applyInternalPreset(2)
            }
            ThemePreviewThumbnail {
                readonly property var d: helper.internalPresets[0]
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
                readonly property var d: helper.internalPresets[1]
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
                readonly property var d: helper.internalPresets[2]
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
        Label {
            Layout.alignment: Qt.AlignHCenter
            text: qsTr("You can also create a custom color theme later in Settings > Appearance > Color Scheme")
        }
    }
}