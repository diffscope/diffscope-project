// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_BATCHPROCESS_SHELLINTERFACE_P_H
#define DIFFSCOPE_BATCHPROCESS_SHELLINTERFACE_P_H

#include <batchprocess/ShellInterface.h>

#include <QPointer>

namespace BatchProcess {

    namespace Internal {
        class ShellModuleExtension;
    }

    class ShellInterfacePrivate {
        Q_DECLARE_PUBLIC(ShellInterface)
    public:
        explicit ShellInterfacePrivate(ShellInterface *q);

        ShellInterface *q_ptr;
        QPointer<Internal::ShellModuleExtension> moduleExtension;
    };

}

#endif // DIFFSCOPE_BATCHPROCESS_SHELLINTERFACE_P_H
