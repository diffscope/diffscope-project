// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

import QtQuick
import QtQuick.Controls.impl

import SVSCraft.UIComponents

IconImage {
    required property int severity
    property int size: 16
    property bool regular: false

    sourceSize.width: size
    sourceSize.height: size
    source: "image://fluent-system-icons/" + (severity === DocumentProblems.Error ? "error_circle"
        : severity === DocumentProblems.Warning ? "warning" : "info")
        + "?size=" + size + (regular ? "&style=regular" : "")
    color: severity === DocumentProblems.Error ? Theme.errorColor : Theme.warningColor
}
