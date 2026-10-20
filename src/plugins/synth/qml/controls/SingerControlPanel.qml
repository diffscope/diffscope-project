// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

import QtQml
import QtQuick

import DiffScope.Core
import DiffScope.Synth

ExtraControlPanel {
    id: control

    required property string architectureId
    property string singerId: ""

    signal extraEdited()

    panelComponent: provider.singerControlPanelComponent
    providerLoading: provider.loading
    providerFailed: provider.failed
    needsInfo: true
    infoAvailable: singerInfoProvider.exists
    info: singerInfoProvider.info.userData
    onStateEdited: extraEdited()

    ControlPanelProvider {
        id: provider
        architectureId: control.architectureId
    }

    SingerInfoProvider {
        id: singerInfoProvider
        registry: CoreInterface.singerRegistry
        architectureId: control.architectureId
        singerId: control.singerId
    }

    function setSingerId(value) {
        singerId = value
    }

    function setExtra(value) {
        loadState(value)
    }

    function getExtra() {
        return saveState()
    }
}
