// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

import QtQml
import QtQuick
import QtQuick.Controls

import QActionKit

import SVSCraft.UIComponents

DockingPane {
    title: ActionInstantiator.text
    description: ActionInstantiator.description
    icon.source: ActionInstantiator.icon.source
    icon.color: ActionInstantiator.icon.color.valid ? ActionInstantiator.icon.color : Theme.foregroundPrimaryColor
}
