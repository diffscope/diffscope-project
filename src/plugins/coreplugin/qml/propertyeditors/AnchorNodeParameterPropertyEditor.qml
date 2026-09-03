// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

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
            columnItem: DoubleSpinBox {
                id: valueSpinBox
                readonly property var normalizedValue: parameterInfoProvider.valueFromDspx(
                    groupBox.propertyMapper?.value)
                readonly property var displayValue: {
                    const info = parameterInfoProvider.info
                    if (!parameterInfoProvider.exists || info === undefined || info === null)
                        return undefined
                    return parameterInfoProvider.displayValue(normalizedValue)
                }
                readonly property var mappedBottomDisplayValue: parameterInfoProvider.displayValue(0.0)
                readonly property var mappedTopDisplayValue: parameterInfoProvider.displayValue(1.0)
                readonly property double bottomDisplayValue: Number.isFinite(mappedBottomDisplayValue)
                    ? mappedBottomDisplayValue : (mappedBottomDisplayValue < 0 ? -999.0 : 999.0)
                readonly property double topDisplayValue: Number.isFinite(mappedTopDisplayValue)
                    ? mappedTopDisplayValue : (mappedTopDisplayValue < 0 ? -999.0 : 999.0)

                enabled: parameterInfoProvider.exists
                decimals: 3
                from: Math.min(bottomDisplayValue, topDisplayValue)
                to: Math.max(bottomDisplayValue, topDisplayValue)
                value: {
                    if (displayValue === undefined || displayValue === null)
                        return 0
                    const lowerBound = Math.min(from, to)
                    const upperBound = Math.max(from, to)
                    return Math.max(lowerBound, Math.min(upperBound, displayValue))
                }
                stepSize: 0.001
                contentItem.visible: displayValue !== undefined && displayValue !== null
                textFromValue: function(value, decimals, locale) {
                    if (normalizedValue === 0.0 && !Number.isFinite(mappedBottomDisplayValue)
                            && Math.abs(value - from) < stepSize / 2.0) {
                        return "−∞"
                    }
                    return Number(value).toLocaleString(locale, "f", decimals)
                }
                valueFromText: function(text, locale) {
                    const valueText = String(text).trim()
                    if (valueText === "−∞" || valueText === "-∞")
                        return from
                    const parsed = Number.fromLocaleString(locale, valueText)
                    return isNaN(parsed) ? 0 : parsed
                }

                onValueModified: {
                    const normalized = !Number.isFinite(mappedBottomDisplayValue)
                            && Math.abs(value - from) < stepSize / 2.0
                        ? 0.0 : parameterInfoProvider.valueFromDisplay(value)
                    if (normalized === undefined || normalized === null)
                        return
                    if (!spinBoxHelper.buttonPressed)
                        groupBox.beginTransaction()
                    if (!groupBox.transactionId)
                        return
                    groupBox.propertyMapper.value = parameterInfoProvider.valueToDspx(normalized)
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
