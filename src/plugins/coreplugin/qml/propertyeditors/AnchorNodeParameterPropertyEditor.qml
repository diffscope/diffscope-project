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
                readonly property var displayValue: {
                    const info = parameterInfoProvider.info
                    if (!parameterInfoProvider.exists || info === undefined || info === null)
                        return undefined
                    return parameterInfoProvider.displayValue(groupBox.propertyMapper?.value)
                }
                readonly property var bottomDisplayValue: {
                    const info = parameterInfoProvider.info
                    if (!parameterInfoProvider.exists)
                        return undefined
                    return parameterInfoProvider.displayValue(info.bottomValue)
                }
                readonly property var topDisplayValue: {
                    const info = parameterInfoProvider.info
                    if (!parameterInfoProvider.exists)
                        return undefined
                    return parameterInfoProvider.displayValue(info.topValue)
                }

                enabled: parameterInfoProvider.exists
                decimals: 3
                from: Math.min(bottomDisplayValue ?? 0, topDisplayValue ?? 0)
                to: Math.max(bottomDisplayValue ?? 0, topDisplayValue ?? 0)
                value: {
                    if (displayValue === undefined || displayValue === null)
                        return 0
                    const lowerBound = Math.min(from, to)
                    const upperBound = Math.max(from, to)
                    return Math.max(lowerBound, Math.min(upperBound, displayValue))
                }
                stepSize: 0.001
                contentItem.visible: displayValue !== undefined && displayValue !== null

                onValueModified: {
                    const rawValue = parameterInfoProvider.rawValue(value)
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
