pragma ComponentBehavior: Bound

import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl
import QtQuick.Layouts

import SVSCraft
import SVSCraft.UIComponents
import SVSCraft.UIComponents.impl

import DiffScope.UIShell
import DiffScope.DspxModel as DspxModel

PropertyEditorGroupBox {
    id: groupBox
    required property ProjectWindowInterface windowHandle
    required property QtObject propertyMapper

    readonly property QtObject selectionModel:
        windowHandle?.projectDocumentContext.document.selectionModel ?? null
    readonly property QtObject anchorNodeSequence:
        selectionModel?.anchorNodeSelectionModel.anchorNodeSequenceWithSelectedItems ?? null
    readonly property QtObject parameter: anchorNodeSequence?.parameter ?? null
    readonly property QtObject parameterMap: parameter?.parameterMap ?? null
    readonly property QtObject singingClip: parameterMap?.singingClip ?? null
    readonly property string parameterId: {
        const map = parameterMap
        if (!map || !parameter)
            return ""
        for (const key of map.keys) {
            if (map.item(key) === parameter)
                return key
        }
        return ""
    }
    property int transactionId: 0

    title: qsTr("Parameter Anchor")

    function beginTransaction(): bool {
        transactionId = windowHandle.projectDocumentContext.document.transactionController.beginTransaction()
        return transactionId !== 0
    }

    function commitTransaction() {
        windowHandle.projectDocumentContext.document.transactionController.commitTransaction(
            transactionId, qsTr("Editing parameter anchor"))
        transactionId = 0
    }

    ParameterInfoProvider {
        id: parameterInfoProvider
        registry: CoreInterface.singerRegistry
        architectureId: groupBox.singingClip?.sources?.category ?? ""
        parameterId: groupBox.parameterId
        transform: groupBox.anchorNodeSequence?.role === DspxModel.AnchorNodeSequence.Transform
    }

    ColumnLayout {
        width: parent.width

        RowLayout {
            Layout.fillWidth: true
            visible: !parameterInfoProvider.exists
            IconImage {
                source: "image://fluent-system-icons/warning"
                sourceSize: Qt.size(16, 16)
                color: Theme.foregroundPrimaryColor
            }
            Label {
                Layout.fillWidth: true
                text: qsTr("The selected parameter is not registered for this singer architecture.")
                wrapMode: Text.Wrap
            }
        }

        FormGroup {
            Layout.fillWidth: true
            label: qsTr("Position")
            columnItem: TextField {
                text: groupBox.propertyMapper?.position === undefined ? "" :
                    GlobalHelper.musicTimelineTextFromValue(
                        groupBox.windowHandle?.projectTimeline.musicTimeline ?? null,
                        groupBox.propertyMapper.position, 1, 1, 3)
                readOnly: true
                ThemedItem.flat: true
            }
        }

        FormGroup {
            Layout.fillWidth: true
            label: qsTr("Value")
            columnItem: SpinBox {
                id: valueSpinBox
                readonly property int decimals: 3
                readonly property int decimalFactor: Math.pow(10, decimals)
                readonly property var displayValue:
                    parameterInfoProvider.displayValue(groupBox.propertyMapper?.value)
                readonly property var bottomDisplayValue:
                    parameterInfoProvider.displayValue(parameterInfoProvider.info.bottomValue)
                readonly property var topDisplayValue:
                    parameterInfoProvider.displayValue(parameterInfoProvider.info.topValue)

                enabled: parameterInfoProvider.exists
                from: Math.round(Math.min(bottomDisplayValue ?? 0, topDisplayValue ?? 0)
                                 * decimalFactor)
                to: Math.round(Math.max(bottomDisplayValue ?? 0, topDisplayValue ?? 0)
                               * decimalFactor)
                value: displayValue === undefined || displayValue === null
                    ? 0 : Math.round(displayValue * decimalFactor)
                stepSize: 1
                contentItem.visible: displayValue !== undefined && displayValue !== null
                validator: DoubleValidator {
                    locale: valueSpinBox.locale.name
                    bottom: Math.min(valueSpinBox.bottomDisplayValue ?? 0,
                                     valueSpinBox.topDisplayValue ?? 0)
                    top: Math.max(valueSpinBox.bottomDisplayValue ?? 0,
                                  valueSpinBox.topDisplayValue ?? 0)
                    decimals: valueSpinBox.decimals
                    notation: DoubleValidator.StandardNotation
                }

                textFromValue: function(value, locale) {
                    return Number(value / decimalFactor).toLocaleString(
                        locale, "f", decimals)
                }
                valueFromText: function(text, locale) {
                    return Math.round(Number.fromLocaleString(locale, text) * decimalFactor)
                }

                onValueModified: {
                    const rawValue = parameterInfoProvider.rawValue(value / decimalFactor)
                    if (rawValue === undefined || rawValue === null)
                        return
                    if (!spinBoxHelper.buttonPressed)
                        groupBox.beginTransaction()
                    if (!groupBox.transactionId)
                        return
                    groupBox.propertyMapper.value = rawValue
                    if (!spinBoxHelper.buttonPressed)
                        groupBox.commitTransaction()
                }

                SpinBoxPressedHelper {
                    id: spinBoxHelper
                    spinBox: valueSpinBox
                    onPressed: groupBox.beginTransaction()
                    onReleased: groupBox.commitTransaction()
                }
            }
        }

        FormGroup {
            Layout.fillWidth: true
            label: qsTr("Interpolation")
            columnItem: ComboBox {
                id: interpolationCombo
                enabled: parameterInfoProvider.exists
                textRole: "text"
                valueRole: "value"
                model: [
                    { text: qsTr("None"), value: DspxModel.AnchorNode.None },
                    { text: qsTr("Linear"), value: DspxModel.AnchorNode.Linear },
                    { text: qsTr("Hermite"), value: DspxModel.AnchorNode.Hermite }
                ]
                Component.onCompleted: currentValue = Qt.binding(() => groupBox.propertyMapper?.interpolationMode)
                onActivated: index => {
                    if (!groupBox.beginTransaction())
                        return
                    groupBox.propertyMapper.interpolationMode = valueAt(index)
                    groupBox.commitTransaction()
                }
            }
        }
    }
}
