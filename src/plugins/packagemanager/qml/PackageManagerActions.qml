// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

import QtQml
import QtQuick
import QtQuick.Controls

import QActionKit

import DiffScope.PackageManager

ActionCollection {
    id: d

    required property PackageManagerAddOn addOn

    ActionItem {
        actionId: "org.diffscope.packagemanager.packageManager"
        Action {
            onTriggered: () => {
                Qt.callLater(() => d.addOn.openPackageManager())
            }
        }
    }
}
