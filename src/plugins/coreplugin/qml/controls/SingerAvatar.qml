import QtQml
import QtQuick
import QtQuick.Controls.impl

import SVSCraft
import SVSCraft.UIComponents

Item {
    id: control

    property string architectureId
    property var singerTree: []

    Component {
        id: singleSingerAvatarComponent

        Item {
            id: singleSingerAvatar

            property string architectureId
            property string singerId

            readonly property url avatarUrl: singerInfoProvider.info.avatarUrl
            readonly property bool hasAvatarUrl: singerInfoProvider.exists
                                                  && String(avatarUrl).length > 0

            clip: true

            SingerInfoProvider {
                id: singerInfoProvider
                registry: CoreInterface.singerRegistry
                architectureId: singleSingerAvatar.architectureId
                singerId: singleSingerAvatar.singerId
            }

            Image {
                anchors.fill: parent
                visible: singleSingerAvatar.hasAvatarUrl
                source: visible ? singleSingerAvatar.avatarUrl : ""
                fillMode: Image.PreserveAspectCrop
                mipmap: true
                asynchronous: true
            }

            Rectangle {
                id: fallbackAvatar

                anchors.fill: parent
                visible: !singleSingerAvatar.hasAvatarUrl
                color: Theme.backgroundQuaternaryColor

                readonly property real iconSize: Math.min(width, height) * 2 / 3

                IconLabel {
                    anchors.centerIn: parent
                    icon.source: `image://fluent-system-icons/${singerInfoProvider.exists ? "mic" : "question"}?size=32&style=regular`
                    icon.color: Theme.foregroundSecondaryColor
                    icon.width: Math.max(1, fallbackAvatar.iconSize)
                    icon.height: Math.max(1, fallbackAvatar.iconSize)
                }
            }
        }
    }

    Component {
        id: avatarNodeComponent

        Item {
            id: avatarNode

            property string architectureId
            property var singerNode

            readonly property bool isSingleSinger: typeof singerNode === "string"
            readonly property bool isSingerGroup: !isSingleSinger
                                                  && singerNode !== null
                                                  && singerNode !== undefined
                                                  && typeof singerNode.length === "number"
            readonly property var childSingers: isSingerGroup
                                                ? Array.prototype.slice.call(singerNode, 0, 4)
                                                : []
            readonly property int childCount: childSingers.length
            readonly property real childSize: childCount === 1 ? width : width * 0.44

            Loader {
                id: singleSingerAvatarLoader
                anchors.fill: parent
                active: avatarNode.isSingleSinger
                sourceComponent: singleSingerAvatarComponent

                Binding {
                    target: singleSingerAvatarLoader.item
                    property: "architectureId"
                    value: avatarNode.architectureId
                    when: singleSingerAvatarLoader.status === Loader.Ready
                }

                Binding {
                    target: singleSingerAvatarLoader.item
                    property: "singerId"
                    value: avatarNode.isSingleSinger ? avatarNode.singerNode : ""
                    when: singleSingerAvatarLoader.status === Loader.Ready
                }
            }

            Repeater {
                model: avatarNode.childSingers

                delegate: Loader {
                    id: childAvatarLoader

                    required property int index
                    required property var modelData

                    width: avatarNode.childSize
                    height: width
                    x: {
                        if (avatarNode.childCount === 1)
                            return 0
                        if (avatarNode.childCount === 3 && index === 2)
                            return avatarNode.width * 0.28
                        return avatarNode.width * (index % 2 === 0 ? 0.04 : 0.52)
                    }
                    y: {
                        if (avatarNode.childCount === 1)
                            return 0
                        if (avatarNode.childCount === 2)
                            return avatarNode.height * 0.28
                        return avatarNode.height * (index < 2 ? 0.04 : 0.52)
                    }
                    sourceComponent: avatarNodeComponent

                    Binding {
                        target: childAvatarLoader.item
                        property: "architectureId"
                        value: avatarNode.architectureId
                        when: childAvatarLoader.status === Loader.Ready
                    }

                    Binding {
                        target: childAvatarLoader.item
                        property: "singerNode"
                        value: childAvatarLoader.modelData
                        when: childAvatarLoader.status === Loader.Ready
                    }
                }
            }
        }
    }

    Loader {
        id: rootAvatarLoader
        width: Math.min(control.width, control.height)
        height: width
        anchors.centerIn: parent
        sourceComponent: avatarNodeComponent

        Binding {
            target: rootAvatarLoader.item
            property: "architectureId"
            value: control.architectureId
            when: rootAvatarLoader.status === Loader.Ready
        }

        Binding {
            target: rootAvatarLoader.item
            property: "singerNode"
            value: control.singerTree
            when: rootAvatarLoader.status === Loader.Ready
        }
    }
}
