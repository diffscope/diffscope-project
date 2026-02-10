import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Templates as T

import SVSCraft
import SVSCraft.UIComponents

import DiffScope.UIShell
import DiffScope.DspxModel as DspxModel

PropertyEditorGroupBox {
    id: groupBox
    required property ProjectWindowInterface windowHandle
    required property QtObject propertyMapper
    title: qsTr("Basic")
    ColumnLayout {
        width: parent.width
        TextPropertyEditorField {
            windowHandle: groupBox.windowHandle
            propertyMapper: groupBox.propertyMapper
            key: "name"
            label: qsTr("Name")
            transactionName: qsTr("Renaming clip")
        }
        FormGroup {
            Layout.fillWidth: true
            label: qsTr("Type")
            columnItem: Label {
                text: groupBox.propertyMapper?.type === 0 ? qsTr("Audio") : groupBox.propertyMapper?.type === 1 ? qsTr("Singing") : qsTr("Multiple types")
            }
        }
        FormGroup {
            id: assiciatedTrackSelector
            Layout.fillWidth: true
            label: qsTr("Associated track")
            property int transactionId: 0
            function beginTransaction() {
                let a = groupBox.windowHandle.projectDocumentContext.document.transactionController.beginTransaction()
                if (a) {
                    transactionId = a
                    return true
                }
                return false
            }
            function commitTransaction() {
                groupBox.windowHandle.projectDocumentContext.document.transactionController.commitTransaction(transactionId, "Changing associated track of clip")
                transactionId = 0
            }
            rowItem: ToolButton {
                text: qsTr("Select track")
                flat: true
                display: AbstractButton.IconOnly
                icon.source: "image://fluent-system-icons/location_target_square"
                onClicked: () => {
                    let selectionModel = groupBox.windowHandle.projectDocumentContext.document.selectionModel
                    let tracks = selectionModel.clipSelectionModel.clipSequencesWithSelectedItems.map(v => v.track)
                    for (let track of tracks) {
                        selectionModel.select(track, DspxModel.SelectionModel.Select | DspxModel.SelectionModel.SetCurrentItem)
                    }
                }
            }
            columnItem: ComboBox {
                displayText: groupBox.propertyMapper?.associatedTrack ? undefined : qsTr("Multiple tracks")
                model: groupBox.windowHandle?.projectDocumentContext.document.model.tracks.items.map((track, i) => ({text: qsTr("%L1: %2").arg(i + 1).arg(track.name), value: track})) ?? null
                textRole: "text"
                valueRole: "value"
                currentIndex: indexOfValue(groupBox.propertyMapper?.associatedTrack)
                onActivated: (index) => {
                    if (index === currentIndex)
                        return
                    assiciatedTrackSelector.beginTransaction()
                    if (!assiciatedTrackSelector.transactionId)
                        return
                    groupBox.propertyMapper.associatedTrack = valueAt(index)
                    assiciatedTrackSelector.commitTransaction()
                }
            }
        }
    }
}