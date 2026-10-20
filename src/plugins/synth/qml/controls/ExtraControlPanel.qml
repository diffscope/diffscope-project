// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SVSCraft
import SVSCraft.UIComponents

import "JsonState.js" as JsonState

Item {
    id: control

    property Component panelComponent
    property bool providerLoading: false
    property bool providerFailed: false
    property bool needsInfo: false
    property bool infoAvailable: true
    property var info: null
    property real padding: 12

    property var committedState: null
    property bool stateLoaded: false
    property bool initializing: false
    property bool initializationPending: true
    property bool panelReady: false
    property bool panelFailed: false
    property string failureMessage: ""

    signal stateEdited()

    onPanelComponentChanged: resetPanel()
    onProviderLoadingChanged: resetPanel()
    onProviderFailedChanged: resetPanel()
    onInfoChanged: resetPanel()
    onInfoAvailableChanged: resetPanel()

    function failPanel(message, error) {
        if (error)
            console.warn("Synthesis control panel:", error)
        panelReady = false
        failureMessage = message
        panelFailed = true
    }

    function resetPanel() {
        initializationPending = true
        panelReady = false
        panelFailed = false
        failureMessage = ""
        Qt.callLater(initializePanel)
    }

    function loadState(state) {
        const value = JsonState.copy(state)
        if (stateLoaded && JSON.stringify(value) === JSON.stringify(committedState))
            return
        committedState = value
        stateLoaded = true
        initializationPending = true
        jsonFallback.loadState(value)
        if (panelFailed)
            resetPanel()
        initializePanel()
    }

    function initializePanel() {
        if (!initializationPending || !stateLoaded || !panelLoader.item || panelFailed || providerLoading || providerFailed)
            return
        initializationPending = false
        initializing = true
        panelReady = false
        const panel = panelLoader.item
        try {
            if (!(panel instanceof Item) || typeof panel.loadState !== "function"
                    || typeof panel.saveState !== "function" || typeof panel.validateState !== "function"
                    || typeof panel.stateEdited !== "function")
                throw new Error("The control panel does not implement the state contract.")
            if (needsInfo) {
                try {
                    if (!infoAvailable || typeof panel.validateInfo !== "function" || typeof panel.loadInfo !== "function"
                            || panel.validateInfo(JsonState.copy(info)) === false || panel.loadInfo(JsonState.copy(info)) === false) {
                        failPanel(qsTr("Additional settings are not available for this singer."))
                        return
                    }
                } catch (error) {
                    failPanel(qsTr("Additional settings are not available for this singer."), error)
                    return
                }
            }
            if (panel.validateState(JsonState.copy(committedState)) === false
                    || panel.loadState(JsonState.copy(committedState)) === false) {
                failPanel(qsTr("The current settings cannot be opened in this panel."))
                return
            }
            panelReady = true
        } catch (error) {
            failPanel(qsTr("Additional settings could not be opened."), error)
        } finally {
            initializing = false
        }
    }

    function captureState() {
        if (!panelReady || initializing)
            return true
        initializing = true
        try {
            const value = JsonState.copy(panelLoader.item.saveState())
            if (panelLoader.item.validateState(JsonState.copy(value)) === false)
                throw new Error("The control panel rejected the saved state.")
            committedState = value
            jsonFallback.loadState(value)
            return true
        } catch (error) {
            failPanel(qsTr("The settings could not be applied."), error)
            return false
        } finally {
            initializing = false
        }
    }

    function saveState() {
        return JsonState.copy(committedState)
    }

    function validateState(state) {
        JsonState.copy(state)
    }

    Loader {
        id: panelLoader
        anchors.fill: parent
        anchors.margins: control.padding
        active: Boolean(control.panelComponent) && !control.providerLoading && !control.providerFailed && !control.panelFailed
        sourceComponent: active ? control.panelComponent : null
        visible: control.panelReady
        onLoaded: {
            control.initializationPending = true
            control.initializePanel()
        }
        onStatusChanged: {
            if (status === Loader.Error)
                control.failPanel(qsTr("Additional settings could not be opened."))
        }
    }

    Connections {
        target: panelLoader.item
        ignoreUnknownSignals: true

        function onStateEdited() {
            if (!control.panelReady || control.initializing)
                return
            const previous = JSON.stringify(control.committedState)
            if (control.captureState() && previous !== JSON.stringify(control.committedState))
                control.stateEdited()
        }
    }

    BusyIndicator {
        anchors.centerIn: parent
        visible: control.providerLoading || panelLoader.status === Loader.Loading
        running: visible
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: control.padding
        visible: !control.panelReady && !control.providerLoading && panelLoader.status !== Loader.Loading
        spacing: 8

        Label {
            Layout.fillWidth: true
            text: control.failureMessage || (control.providerFailed
                  ? qsTr("Additional settings could not be opened.")
                  : qsTr("No control panel is available for these settings."))
            wrapMode: Text.Wrap
            ThemedItem.foregroundLevel: SVS.FL_Secondary
        }

        Label {
            id: singerInformationLabel
            Layout.fillWidth: true
            visible: control.needsInfo
            text: qsTr("Singer Information")
            font.weight: Font.DemiBold
        }

        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: 0
            Layout.preferredHeight: 120
            visible: control.needsInfo

            TextArea {
                text: JSON.stringify(control.info ?? null, null, 4)
                readOnly: true
                selectByMouse: true
                textFormat: TextEdit.PlainText
                wrapMode: TextEdit.NoWrap
                Accessible.labelledBy: singerInformationLabel
            }
        }

        Label {
            Layout.fillWidth: true
            visible: control.needsInfo
            text: qsTr("Settings")
            font.weight: Font.DemiBold
        }

        JsonExtraEditor {
            id: jsonFallback
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.preferredHeight: 240

            onStateEdited: {
                control.committedState = jsonFallback.saveState()
                control.stateEdited()
            }
        }
    }
}
