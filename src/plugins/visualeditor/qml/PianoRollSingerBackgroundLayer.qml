// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

import QtQuick
import QtQuick.Layouts

import DiffScope.Core
import DiffScope.SingerCatalog

Item {
    id: singerBackgroundLayer

    readonly property PianoRollPanelInterface pianoRollPanelInterface: ViewLayer.pianoRollPanelInterface
    readonly property var singerBackgroundIds:
        flattenUniqueSingerIds(singerBackgroundIdProvider.singerTree)

    function flattenUniqueSingerIds(singerTree) {
        const singerIds = []

        function appendSingerIds(node) {
            if (typeof node === "string") {
                if (node !== "" && !singerIds.includes(node))
                    singerIds.push(node)
                return
            }
            if (!node || typeof node.length !== "number")
                return
            for (let index = 0; index < node.length; ++index)
                appendSingerIds(node[index])
        }

        appendSingerIds(singerTree)
        return singerIds
    }

    ClipSingerIdProvider {
        id: singerBackgroundIdProvider
        sources: singerBackgroundLayer.pianoRollPanelInterface?.editingClip?.sources ?? null
    }

    RowLayout {
        id: singerBackgroundRow

        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.topMargin: EditorPreference.singerBackgroundVerticalMargin
        anchors.bottomMargin: EditorPreference.singerBackgroundVerticalMargin
        x: {
            switch (EditorPreference.singerBackgroundAlignment) {
            case EditorPreference.SBA_Left:
                return EditorPreference.singerBackgroundHorizontalMargin
            case EditorPreference.SBA_Center:
                return (parent.width - width) / 2
                    + EditorPreference.singerBackgroundHorizontalOffset
            default:
                return parent.width - width
                    - EditorPreference.singerBackgroundHorizontalMargin
            }
        }
        spacing: EditorPreference.singerBackgroundSpacing
        visible: EditorPreference.showSingerBackground
        opacity: EditorPreference.singerBackgroundOpacity

        Repeater {
            model: EditorPreference.showAllSingerBackgrounds
                   ? singerBackgroundLayer.singerBackgroundIds
                   : singerBackgroundLayer.singerBackgroundIds.slice(0, 1)

            delegate: Image {
                id: singerBackgroundImage

                required property string modelData

                readonly property url backgroundUrl:
                    singerInfoProvider.info.backgroundUrl
                readonly property bool hasBackground:
                    singerInfoProvider.exists
                    && String(backgroundUrl).length > 0

                Layout.fillHeight: true
                Layout.preferredWidth: hasBackground
                                       && sourceSize.height > 0
                                       ? singerBackgroundRow.height
                                         * sourceSize.width
                                         / sourceSize.height
                                       : 0

                visible: hasBackground
                source: hasBackground ? backgroundUrl : ""
                fillMode: Image.PreserveAspectFit
                asynchronous: true
                mipmap: true

                SingerInfoProvider {
                    id: singerInfoProvider
                    registry: CoreInterface.singerRegistry
                    architectureId: singerBackgroundIdProvider.architectureId
                    singerId: singerBackgroundImage.modelData
                }
            }
        }
    }
}
