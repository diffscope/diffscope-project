import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SVSCraft
import SVSCraft.UIComponents

import DiffScope.UIShell

FormGroup {
    id: d
    required property ProjectWindowInterface windowHandle
    required property QtObject propertyMapper
    required property string key
    required property string transactionName
    property var spinBoxValueFromProperty: (v) => v
    property var propertyFromSpinBoxValue: (v) => v
    property var sliderValueFromProperty: (v) => v
    property var propertyFromSliderValue: (v) => v
    property double from: 0
    property double to: 2147483647
    property bool useSlider: false
    Layout.fillWidth: true
    property int transactionId: 0
    function beginTransaction() {
        let a = d.windowHandle.projectDocumentContext.document.transactionController.beginTransaction()
        if (a) {
            transactionId = a
            return true
        }
        return false
    }
    function commitTransaction() {
        d.windowHandle.projectDocumentContext.document.transactionController.commitTransaction(transactionId, d.transactionName)
        transactionId = 0
    }
    function abortTransaction() {
        d.windowHandle.projectDocumentContext.document.transactionController.abortTransaction(transactionId)
        transactionId = 0
    }
    readonly property SpinBox spinBox: SpinBox {
        id: spinBox
        value: d.spinBoxValueFromProperty(d.propertyMapper?.[d.key] ?? 0)
        from: d.spinBoxValueFromProperty(d.from)
        to: d.spinBoxValueFromProperty(d.to)
        contentItem.visible: d.propertyMapper?.[d.key] !== undefined
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
        value: d.sliderValueFromProperty(d.propertyMapper?.[d.key] ?? 0)
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
    rowItem: useSlider ? spinBox: null
    columnItem: useSlider ? slider: spinBox
}

