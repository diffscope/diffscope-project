pragma ComponentBehavior: Bound

import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SVSCraft
import SVSCraft.UIComponents
import SVSCraft.UIComponents.impl

import DiffScope.DspxModel as DspxModel
import DiffScope.UIShell

PropertyEditorGroupBox {
    id: groupBox

    required property ProjectWindowInterface windowHandle
    required property QtObject propertyMapper

    property int transactionId: 0

    title: qsTr("Voice blending anchor")

    function beginTransaction(): bool {
        if (transactionId)
            return true
        transactionId = windowHandle.projectDocumentContext.document
            .transactionController.beginTransaction()
        return transactionId !== 0
    }

    function commitTransaction() {
        if (!transactionId)
            return
        windowHandle.projectDocumentContext.document.transactionController
            .commitTransaction(transactionId, qsTr("Editing voice blending"))
        transactionId = 0
    }

    function abortTransaction() {
        if (!transactionId)
            return
        windowHandle.projectDocumentContext.document.transactionController
            .abortTransaction(transactionId)
        transactionId = 0
    }

    function singerNames() {
        const result = []
        for (let index = 0; index < singerRows.count; ++index)
            result.push(singerRows.itemAt(index)?.displayName
                        ?? qsTr("Unnamed singer"))
        return result
    }

    Component.onDestruction: abortTransaction()

    ColumnLayout {
        width: parent.width
        spacing: 8

        FormGroup {
            Layout.fillWidth: true
            label: qsTr("Position")
            columnItem: TextField {
                text: groupBox.propertyMapper?.position === undefined
                      || groupBox.propertyMapper?.position === null ? ""
                    : GlobalHelper.musicTimelineTextFromValue(
                        groupBox.windowHandle?.projectTimeline.musicTimeline ?? null,
                        groupBox.propertyMapper.position, 1, 1, 3)
                readOnly: true
                ThemedItem.flat: true
            }
        }

        Repeater {
            id: singerRows

            model: groupBox.propertyMapper?.singers ?? []

            delegate: RowLayout {
                id: singerRow

                required property int index
                required property var modelData

                Layout.fillWidth: true
                spacing: 8

                readonly property bool mixed:
                    modelData?.type === DspxModel.Singer.Mixed
                readonly property string singerId:
                    mixed ? "" : (modelData?.id ?? "")
                readonly property string displayName: mixed
                    ? qsTr("Mixed singer")
                    : singerInfoProvider.info.name || singerId
                      || qsTr("Unnamed singer")
                readonly property var commonRatios:
                    groupBox.propertyMapper?.ratios
                readonly property var ratioValue: {
                    commonRatios
                    return groupBox.propertyMapper?.ratioAt(index)
                }
                readonly property var maximumValue: {
                    commonRatios
                    const count = groupBox.propertyMapper?.voiceCount ?? 0
                    return count > 0
                        ? groupBox.propertyMapper?.maximumRatioAt(index)
                        : undefined
                }

                SingerInfoProvider {
                    id: singerInfoProvider
                    registry: CoreInterface.singerRegistry
                    architectureId:
                        singerRow.modelData?.singerList?.sources?.category ?? ""
                    singerId: singerRow.singerId
                }

                Label {
                    Layout.fillWidth: true
                    elide: Text.ElideRight
                    text: singerRow.displayName
                }

                SpinBox {
                    id: ratioSpinBox

                    Layout.preferredWidth: 84
                    from: 0
                    to: Math.round(Number(singerRow.maximumValue ?? 0) * 1000)
                    stepSize: 1
                    value: Math.round(Number(singerRow.ratioValue ?? 0) * 1000)
                    enabled: (groupBox.propertyMapper?.voiceCount ?? 0) > 1
                             && singerRow.maximumValue !== undefined
                             && singerRow.maximumValue !== null
                    editable: enabled
                    contentItem.visible: singerRow.ratioValue !== undefined
                                         && singerRow.ratioValue !== null

                    validator: DoubleValidator {
                        locale: ratioSpinBox.locale.name
                        bottom: 0
                        top: ratioSpinBox.to / 10
                        decimals: 1
                        notation: DoubleValidator.StandardNotation
                    }
                    textFromValue: function(value, locale) {
                        return qsTr("%1%").arg(Number(value / 10).toLocaleString(
                                                   locale, "f", 1))
                    }
                    valueFromText: function(text, locale) {
                        const numberText = String(text).replace(/%/g, "").trim()
                        const parsed = Number.fromLocaleString(locale, numberText)
                        return isNaN(parsed) ? 0 : Math.round(parsed * 10)
                    }
                    onValueModified: {
                        const continuous = spinBoxHelper.buttonPressed
                                           || spinBoxHelper.keyPressing
                        if (!continuous && !groupBox.beginTransaction())
                            return
                        if (!groupBox.transactionId)
                            return
                        groupBox.propertyMapper.setSingerRatio(singerRow.index,
                                                               value / 1000)
                        if (!continuous)
                            groupBox.commitTransaction()
                    }

                    SpinBoxPressedHelper {
                        id: spinBoxHelper
                        spinBox: ratioSpinBox
                        onPressed: groupBox.beginTransaction()
                        onReleased: groupBox.commitTransaction()
                    }
                }
            }
        }

        SegmentedRatioSlider {
            Layout.fillWidth: true
            enabled: (groupBox.propertyMapper?.voiceCount ?? 0) > 1
                     && groupBox.propertyMapper?.ratios !== undefined
                     && groupBox.propertyMapper?.ratios !== null
            names: groupBox.singerNames()
            values: groupBox.propertyMapper?.ratios ?? []
            onEditingStarted: groupBox.beginTransaction()
            onAdjacentRatioModified: (leftIndex, leftRatio) => {
                if (groupBox.transactionId)
                    groupBox.propertyMapper.setAdjacentRatios(leftIndex, leftRatio)
            }
            onEditingFinished: groupBox.commitTransaction()
            onEditingCanceled: groupBox.abortTransaction()
        }
    }
}
