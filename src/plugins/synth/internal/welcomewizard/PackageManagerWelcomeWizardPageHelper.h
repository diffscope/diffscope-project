// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_SYNTH_INTERNAL_PACKAGEMANAGERWELCOMEWIZARDPAGEHELPER_H
#define DIFFSCOPE_SYNTH_INTERNAL_PACKAGEMANAGERWELCOMEWIZARDPAGEHELPER_H

#include <QObject>
#include <QWindow>
#include <QtQmlIntegration/qqmlintegration.h>

namespace Synth::Internal {

    class PackageManagerWelcomeWizardPageHelper : public QObject {
        Q_OBJECT
        QML_ELEMENT

    public:
        explicit PackageManagerWelcomeWizardPageHelper(QObject *parent = nullptr);

        Q_INVOKABLE void openPackageManager(QWindow *parent);
    };

}

#endif // DIFFSCOPE_SYNTH_INTERNAL_PACKAGEMANAGERWELCOMEWIZARDPAGEHELPER_H
