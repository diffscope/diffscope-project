// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

import QtQuick.Dialogs

FileDialog {
    title: qsTr("Select Packages")
    fileMode: FileDialog.OpenFiles
    nameFilters: [qsTr("DSPK packages (*.dspk)"), qsTr("All files (*)")]
}
