import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SVSCraft
import SVSCraft.UIComponents

import DiffScope.UIShell

AbstractPropertyEditorField {
    id: d
    property var spinBoxValueFromProperty: (v) => v
    property var propertyFromSpinBoxValue: (v) => v
    property var sliderValueFromProperty: (v) => v
    property var propertyFromSliderValue: (v) => v
    property double from: 0
    property double to: 2147483647
    property bool useSlider: false
    readonly property SpinBox spinBox: spinBox
    readonly property Slider slider: slider
    FormGroup {
        id: formGroup
        Layout.fillWidth: true
        label: d.label
        readonly property SpinBox spinBox: SpinBox {
            id: spinBox
            value: d.spinBoxValueFromProperty(d.value ?? 0)
            from: d.spinBoxValueFromProperty(d.from)
            to: d.spinBoxValueFromProperty(d.to)
            contentItem.visible: d.value !== undefined
            onValueModified: () => {
                if (!spinBoxHelper.buttonPressed) {
                    d.beginTransaction()
                }
                if (!d.transactionId)
                    return
                d.propertyMapper[d.key] = d.propertyFromSpinBoxValue(value)
                if (!spinBoxHelper.buttonPressed) {
                    d.commitTransaction()
                }
            }
            SpinBoxPressedHelper {
                id: spinBoxHelper
                spinBox: spinBox
                onPressed: d.beginTransaction()
                onReleased: d.commitTransaction()
            }
        }
        readonly property Slider slider: Slider {
            id: slider
            value: d.sliderValueFromProperty(d.value ?? 0)
            from: d.sliderValueFromProperty(d.from)
            to: d.sliderValueFromProperty(d.to)
            onMoved: () => {
                if (!pressed) {
                    d.beginTransaction()
                }
                if (!d.transactionId)
                    return
                d.propertyMapper[d.key] = d.propertyFromSliderValue(value)
                if (!pressed) {
                    d.commitTransaction()
                }
            }
            ThemedItem.onDoubleClickReset: moved()
            SliderPressedHelper {
                id: sliderHelper
                slider: slider
                onPressed: d.beginTransaction()
                onReleased: d.commitTransaction()
            }
        }
        rowItem: d.useSlider ? spinBox: null
        columnItem: d.useSlider ? slider: spinBox
    }
}
