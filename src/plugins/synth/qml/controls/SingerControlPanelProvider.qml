// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

pragma ComponentBehavior: Bound

import QtQml

QtObject {
    id: provider

    required property string architectureId

    readonly property Component controlPanelComponent: SingerControlPanel {
        architectureId: provider.architectureId
    }
}
