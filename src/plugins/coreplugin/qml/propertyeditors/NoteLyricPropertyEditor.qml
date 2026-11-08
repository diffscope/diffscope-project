// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SVSCraft
import SVSCraft.UIComponents
import SVSCraft.UIComponents.impl

import DiffScope.UIShell
import DiffScope.Core
import DiffScope.SingerCatalog

PropertyEditorGroupBox {
    id: groupBox
    required property ProjectWindowInterface windowHandle
    required property QtObject propertyMapper
    readonly property var singingClip: propertyMapper?.singingClip ?? null
    readonly property string firstLeafSingerId: findFirstLeafSingerId(clipSingerIdProvider.singerTree)
    readonly property var languageOptions: singerInfoProvider.languageOptions
    title: qsTr("Lyric and Pronunciation")

    function findFirstLeafSingerId(singerTree) {
        if (typeof singerTree === "string")
            return singerTree
        if (!singerTree || typeof singerTree.length !== "number")
            return ""

        for (let index = 0; index < singerTree.length; ++index) {
            const singerId = findFirstLeafSingerId(singerTree[index])
            if (singerId !== "")
                return singerId
        }
        return ""
    }

    ClipSingerIdProvider {
        id: clipSingerIdProvider
        sources: groupBox.singingClip?.sources ?? null
    }

    SingerInfoProvider {
        id: singerInfoProvider
        registry: CoreInterface.singerRegistry
        architectureId: clipSingerIdProvider.architectureId
        singerId: groupBox.firstLeafSingerId
    }

    ColumnLayout {
        width: parent.width

        TextPropertyEditorField {
            windowHandle: groupBox.windowHandle
            propertyMapper: groupBox.propertyMapper
            key: "lyric"
            label: qsTr("Lyric")
            transactionName: qsTr("Editing lyric")
        }

        LanguagePropertyEditorField {
            windowHandle: groupBox.windowHandle
            propertyMapper: groupBox.propertyMapper
            languageOptions: groupBox.languageOptions
            key: "language"
            label: qsTr("Language")
            transactionName: qsTr("Editing language")
        }

        FormGroup {
            Layout.fillWidth: true
            label: qsTr("Pronunciation (original)")
            columnItem: TextField {
                text: groupBox.propertyMapper?.originalPronunciation === undefined
                      ? qsTr("Multiple values")
                      : groupBox.propertyMapper?.originalPronunciation ?? ""
                readOnly: true
                ThemedItem.flat: true
            }
        }

        TextPropertyEditorField {
            windowHandle: groupBox.windowHandle
            propertyMapper: groupBox.propertyMapper
            key: "editedPronunciation"
            label: qsTr("Pronunciation (edited)")
            transactionName: qsTr("Editing pronunciation (edited)")
        }
    }
}
