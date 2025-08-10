import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SVSCraft
import SVSCraft.UIComponents

Item {
    required property QtObject pageHandle
    property bool started: false

    readonly property TextMatcher matcher: TextMatcher {}
}