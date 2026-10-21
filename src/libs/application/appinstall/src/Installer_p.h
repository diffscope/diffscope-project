// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_APPINSTALL_INSTALLER_P_H
#define DIFFSCOPE_APPINSTALL_INSTALLER_P_H

#include <QObject>

#include <appinstall/InstallationTypes.h>

class QWindow;

namespace AppInstall {

    class Installer : public QObject {
        Q_OBJECT
    public:
        static DownloadResult download(const ProductConfiguration &product, QWindow *parent);
        static bool confirmUninstall(const ProductConfiguration &product, const QString &name, const QString &directory, const QString &preparationText, QWindow *parent);
    };

}

#endif // DIFFSCOPE_APPINSTALL_INSTALLER_P_H
