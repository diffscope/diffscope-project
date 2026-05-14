import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SVSCraft
import SVSCraft.UIComponents

import DiffScope.UIShell
import DiffScope.Core

PropertyEditorGroupBox {
    id: groupBox
    required property ProjectWindowInterface windowHandle
    required property QtObject propertyMapper
    readonly property QtObject audioClip: groupBox.windowHandle?.projectDocumentContext.document.selectionModel.clipSelectionModel.selectedItems[0] ?? null
    readonly property QtObject audioClipAudioContext: AudioQmlHelper.getAudioClipAudioContext(groupBox.audioClip)
    title: qsTr("Audio")
    visualVisible: propertyMapper?.type === 0
    StackLayout {
        width: parent.width
        currentIndex: groupBox.windowHandle?.projectDocumentContext.document.selectionModel.selectedCount > 1 ? 0 : 1
        Label {
            text: qsTr("Multiple audio clips")
        }
        ColumnLayout {
            FormGroup {
                id: audioFilePathFormGroup
                readonly property string path: groupBox.audioClip?.path ? AudioQmlHelper.getDisplayAudioFilePath(groupBox.audioClip.path) : ""
                Layout.fillWidth: true
                label: qsTr("Audio file path")
                rowItem: ToolButton {
                    icon.source: "image://fluent-system-icons/open"
                    display: AbstractButton.IconOnly
                    text: qsTr("Reveal in %1").arg(DesktopServices.fileManagerName)
                    enabled: Boolean(audioFilePathFormGroup.path)
                    onClicked: () => {
                        DesktopServices.reveal(audioFilePathFormGroup.path)
                    }
                }
                columnItem: TextField {
                    text: audioFilePathFormGroup.path
                    readOnly: true
                    ThemedItem.flat: true
                }
            }
            Annotation {
                Layout.fillWidth: true
                ThemedItem.controlType: SVS.CT_Error
                label: qsTr("Audio file not specified")
                visible: groupBox.audioClipAudioContext?.status === AudioClipAudioContext.FileNotFound
                         && (!groupBox.audioClip?.path.absoluteDir || !groupBox.audioClip?.path.fileName)
            }
            Annotation {
                Layout.fillWidth: true
                ThemedItem.controlType: SVS.CT_Error
                label: qsTr("Audio file not found")
                visible: groupBox.audioClipAudioContext?.status === AudioClipAudioContext.FileNotFound
            }
            Annotation {
                Layout.fillWidth: true
                ThemedItem.controlType: SVS.CT_Error
                label: qsTr("Audio file failed to load")
                visible: groupBox.audioClipAudioContext?.status === AudioClipAudioContext.FileLoadFailed
            }
            Annotation {
                Layout.fillWidth: true
                ThemedItem.controlType: SVS.CT_Warning
                label: qsTr("Audio file moved")
                visible: groupBox.audioClipAudioContext?.status === AudioClipAudioContext.FileMoved
                display: AbstractButton.TextBesideIcon
                action: Action {
                    text: qsTr("Update Path")
                    onTriggered: () => {
                        AudioQmlHelper.getAudioClipAddOn(groupBox.windowHandle).updateAudioClipToIdenticallyMovedPath(groupBox.audioClip, groupBox.audioClipAudioContext.realAudioPath)
                    }
                }
            }
            FormGroup {
                id: actualPathFormGroup
                visible: groupBox.audioClipAudioContext?.status === AudioClipAudioContext.FileMoved
                readonly property string path: AudioQmlHelper.getDisplayRealPath(groupBox.audioClipAudioContext?.realAudioPath ?? "")
                Layout.fillWidth: true
                label: qsTr("Actual path")
                rowItem: ToolButton {
                    icon.source: "image://fluent-system-icons/open"
                    display: AbstractButton.IconOnly
                    text: qsTr("Reveal in %1").arg(DesktopServices.fileManagerName)
                    enabled: Boolean(actualPathFormGroup.path)
                    onClicked: () => {
                        DesktopServices.reveal(actualPathFormGroup.path)
                    }
                }
                columnItem: TextField {
                    text: actualPathFormGroup.path
                    readOnly: true
                    ThemedItem.flat: true
                }
            }
            Annotation {
                Layout.fillWidth: true
                ThemedItem.controlType: SVS.CT_Warning
                label: qsTr("Audio file content changed")
                visible: groupBox.audioClipAudioContext?.status === AudioClipAudioContext.FileContentChanged
                         && Boolean(groupBox.audioClip?.path.sha512)
                display: AbstractButton.TextBesideIcon
                action: Action {
                    text: qsTr("Confirm the Change")
                    onTriggered: () => {
                        AudioQmlHelper.getAudioClipAddOn(groupBox.windowHandle).updateAudioClipDigest(groupBox.audioClip)
                    }
                }
            }
            Annotation {
                Layout.fillWidth: true
                ThemedItem.controlType: SVS.CT_Warning
                label: qsTr("Audio file content consistency unknown")
                visible: groupBox.audioClipAudioContext?.status === AudioClipAudioContext.FileContentChanged
                         && !groupBox.audioClip?.path.sha512
                display: AbstractButton.TextBesideIcon
                action: Action {
                    text: qsTr("Confirm Current File")
                    onTriggered: () => {
                        AudioQmlHelper.getAudioClipAddOn(groupBox.windowHandle).updateAudioClipDigest(groupBox.audioClip)
                    }
                }
            }
        }
    }
}
