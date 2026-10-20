// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

import QtQml
import QtQml.Models
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Controls.impl
import QtQuick.Templates as T

import SVSCraft
import SVSCraft.UIComponents
import SVSCraft.UIComponents.impl

import DiffScope.UIShell

Window {
    id: dialog
    flags: Qt.Dialog | Qt.CustomizeWindowHint | Qt.WindowTitleHint | Qt.WindowSystemMenuHint | Qt.WindowCloseButtonHint
    modality: Qt.ApplicationModal
    width: 800
    height: 500
    title: qsTr("Welcome")
    property url banner: ""
    color: Theme.backgroundPrimaryColor
    property list<Item> pages: []
    property list<Item> pagesWithHeader: []
    property bool transitioningToWizard: false
    signal finished()

    function enterWizard() {
        if (mainStackLayout.currentIndex !== 0) {
            return
        }
        dialog.transitioningToWizard = true
        welcomeIntroAnimation.stop()
        welcomeExitAnimation.stop()
        welcomeText.opacity = 0
        welcomeBanner.opacity = 0
        mainStackLayout.currentIndex = 1
        const page = dialog.pagesWithHeader.shift()
        pagesStackView.push(page, StackView.Immediate)
        wizardEnterAnimation.start()
    }

    function finishWizardEnterAnimation() {
        wizardEnterAnimation.stop()
        pagesStackView.opacity = 1
        dialog.transitioningToWizard = false
    }

    onClosing: {
        const currentPage = pagesStackView.currentItem?.page
        if (currentPage instanceof WelcomeWizardPage) {
            currentPage.aboutToLeave()
        }
        finished()
    }

    Component {
        id: pageWithHeaderComponent
        Item {
            id: pageWithHeader
            required property Item page
            StackView.onActivating: {
                if (page instanceof WelcomeWizardPage) {
                    page.aboutToEnter()
                }
            }
            Rectangle {
                anchors.fill: parent
                color: Theme.backgroundPrimaryColor
            }
            ColumnLayout {
                anchors.fill: parent
                spacing: 4
                Label {
                    Layout.alignment: Qt.AlignHCenter
                    Layout.topMargin: 8
                    text: pageWithHeader.page.title
                    font.pixelSize: 20
                }
                Label {
                    Layout.alignment: Qt.AlignHCenter
                    text: pageWithHeader.page.description
                    font.pixelSize: 16
                }
                StackLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    data: [pageWithHeader.page]
                }
            }
        }
    }

    Component.onCompleted: () => {
        for (let page of pages) {
            let pageWithHeader = pageWithHeaderComponent.createObject(null, {page})
            pagesWithHeader.push(pageWithHeader)
        }
    }

    ColumnLayout {
        anchors.fill: parent
        StackLayout {
            id: mainStackLayout
            Layout.fillWidth: true
            Layout.fillHeight: true
            Item {
                id: welcomePage

                SequentialAnimation {
                    id: welcomeIntroAnimation
                    running: dialog.visible && welcomePage.visible && !dialog.transitioningToWizard
                    OpacityAnimator {
                        target: welcomeBanner
                        from: 0
                        to: 1
                        duration: 1000
                        easing.type: Easing.InCubic
                    }
                    PauseAnimation {
                        // Schedule phases on the GUI thread because opacity animators require synchronization when they start.
                        duration: 0
                    }
                    ParallelAnimation {
                        ScaleAnimator {
                            target: welcomeBanner
                            from: 1.5
                            to: 1
                            duration: 1000
                            easing.type: Easing.InOutCubic
                        }
                        YAnimator {
                            target: welcomeBanner
                            from: welcomeBanner.initialY
                            to: 0
                            duration: 1000
                            easing.type: Easing.InOutCubic
                        }
                        OpacityAnimator {
                            target: welcomeText
                            from: 0
                            to: 1
                            duration: 1000
                            easing.type: Easing.Linear
                        }
                    }
                }

                ParallelAnimation {
                    id: welcomeExitAnimation
                    OpacityAnimator {
                        target: welcomeText
                        from: welcomeText.opacity
                        to: 0
                        duration: 1000
                        easing.type: Easing.InCubic
                    }
                    ScaleAnimator {
                        target: welcomeBanner
                        from: welcomeBanner.scale
                        to: 2
                        duration: 1000
                        easing.type: Easing.InCubic
                    }
                    YAnimator {
                        target: welcomeBanner
                        from: welcomeBanner.y
                        to: welcomeBanner.initialY
                        duration: 1000
                        easing.type: Easing.InCubic
                    }
                    OpacityAnimator {
                        target: welcomeBanner
                        from: welcomeBanner.opacity
                        to: 0
                        duration: 1000
                        easing.type: Easing.InCubic
                    }
                    onFinished: dialog.enterWizard()
                }

                ColumnLayout {
                    id: welcomeContent
                    anchors.centerIn: parent
                    spacing: 32
                    Item {
                        id: bannerContainer
                        Layout.alignment: Qt.AlignHCenter
                        Layout.preferredHeight: 128
                        Layout.preferredWidth: 128 / welcomeBanner.implicitHeight * welcomeBanner.implicitWidth
                        Image {
                            id: welcomeBanner
                            readonly property real initialY: welcomePage.height / 2 - welcomeContent.y - bannerContainer.y - height / 2
                            anchors.horizontalCenter: parent.horizontalCenter
                            y: initialY
                            source: dialog.banner
                            height: 128
                            width: height / implicitHeight * implicitWidth
                            scale: 1.5
                            opacity: 0
                            fillMode: Image.PreserveAspectFit
                            mipmap: true
                            Accessible.role: Accessible.Graphic
                            Accessible.name: qsTr("Logo of %1").arg(Application.displayName)
                        }
                    }
                    ColumnLayout {
                        id: welcomeText
                        Layout.alignment: Qt.AlignHCenter
                        opacity: 0
                        spacing: 32
                        Label {
                            Layout.alignment: Qt.AlignHCenter
                            text: qsTr("Welcome")
                            font.pixelSize: 32
                        }
                        Label {
                            Layout.alignment: Qt.AlignHCenter
                            text: qsTr("This wizard will help you complete the initial configuration")
                            font.pixelSize: 16
                        }
                    }
                }
            }
            StackView {
                id: pagesStackView
                opacity: 0
                OpacityAnimator {
                    id: wizardEnterAnimation
                    target: pagesStackView
                    from: 0
                    to: 1
                    duration: 1000
                    easing.type: Easing.OutCubic
                    onFinished: dialog.transitioningToWizard = false
                }
            }
        }
        Rectangle {
            Layout.fillWidth: true
            color: Theme.backgroundSecondaryColor
            implicitHeight: 60
            RowLayout {
                anchors.centerIn: parent
                spacing: 8
                Button {
                    implicitWidth: 160
                    visible: mainStackLayout.currentIndex !== 0
                    text: qsTr("Previous")
                    enabled: pagesStackView.depth > 1
                    onClicked: () => {
                        dialog.finishWizardEnterAnimation()
                        const currentPage = pagesStackView.currentItem?.page
                        if (currentPage instanceof WelcomeWizardPage) {
                            currentPage.aboutToLeave()
                        }
                        let page = pagesStackView.pop()
                        dialog.pagesWithHeader.unshift(page)
                    }
                }
                Button {
                    implicitWidth: 160
                    ThemedItem.controlType: SVS.CT_Accent
                    text: mainStackLayout.currentIndex === 0 ? qsTr("Continue") : dialog.pagesWithHeader.length !== 0 ? qsTr("Next") : qsTr("Finish")
                    onClicked: {
                        if (mainStackLayout.currentIndex !== 0) {
                            dialog.finishWizardEnterAnimation()
                        }
                        if (dialog.pagesWithHeader.length === 0) {
                            dialog.close()
                            return
                        }
                        if (mainStackLayout.currentIndex === 0) {
                            if (welcomeIntroAnimation.running || welcomeExitAnimation.running) {
                                dialog.enterWizard()
                                return
                            }
                            dialog.transitioningToWizard = true
                            welcomeIntroAnimation.stop()
                            welcomeExitAnimation.start()
                            return
                        }
                        const currentPage = pagesStackView.currentItem?.page
                        if (currentPage instanceof WelcomeWizardPage) {
                            currentPage.aboutToLeave()
                        }
                        let page = dialog.pagesWithHeader.shift()
                        pagesStackView.push(page)
                    }
                }
            }
        }
    }

}
