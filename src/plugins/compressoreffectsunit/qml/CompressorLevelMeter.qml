// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

import QtQuick

import dev.sjimo.ScopicFlow

Item {
    id: meter

    property double from: -48
    property bool reversed: false
    property double to: 0
    property double value: from

    readonly property double normalizedValue: Math.min(
        Math.max((value - from) / (to - from), 0), 1)

    implicitWidth: 7

    Rectangle {
        anchors.fill: parent
        color: SFPalette.levelMeterColor

        Rectangle {
            anchors.fill: parent
            gradient: Gradient {
                orientation: Gradient.Vertical
                GradientStop {
                    color: meter.reversed ? SFPalette.levelLowColor : SFPalette.levelHighColor
                    position: 0
                }
                GradientStop {
                    color: meter.reversed ? SFPalette.levelLowColor : SFPalette.levelMediumColor
                    position: 0.35
                }
                GradientStop {
                    color: meter.reversed ? SFPalette.levelMediumColor : SFPalette.levelLowColor
                    position: 0.65
                }
                GradientStop {
                    color: meter.reversed ? SFPalette.levelHighColor : SFPalette.levelLowColor
                    position: 1
                }
            }
        }

        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: meter.reversed ? undefined : parent.top
            anchors.bottom: meter.reversed ? parent.bottom : undefined
            color: SFPalette.levelMeterColor
            height: (1 - meter.normalizedValue) * parent.height
        }
    }
}
