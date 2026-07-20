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

    function updateValueText() {
        if (valueField.activeFocus) {
            const value = parameterInfoProvider.displayValue(propertyMapper?.value)
            valueField.text = value === undefined || value === null
                ? "" : Number(value).toLocaleString()
        } else {
            valueField.text = parameterInfoProvider.displayString(propertyMapper?.value)
        }
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
            columnItem: TextField {
                id: valueField
                enabled: parameterInfoProvider.exists
                validator: DoubleValidator {
                    notation: DoubleValidator.StandardNotation
                }
                onActiveFocusChanged: {
                    if (activeFocus)
                        groupBox.updateValueText()
                }
                onEditingFinished: {
                    const displayValue = Number.fromLocaleString(Qt.locale(), text)
                    const rawValue = parameterInfoProvider.rawValue(displayValue)
                    if (rawValue === undefined || rawValue === null || !groupBox.beginTransaction()) {
                        groupBox.updateValueText()
                        return
                    }
                    groupBox.propertyMapper.value = rawValue
                    groupBox.commitTransaction()
                    groupBox.updateValueText()
                }
                Component.onCompleted: groupBox.updateValueText()
                Connections {
                    target: groupBox.propertyMapper
                    function onValueChanged() { groupBox.updateValueText() }
                }
                Connections {
                    target: parameterInfoProvider
                    function onInfoChanged() { groupBox.updateValueText() }
                    function onExistsChanged() { groupBox.updateValueText() }
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
