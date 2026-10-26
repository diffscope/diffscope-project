// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#ifndef DIFFSCOPE_COREPLUGIN_FILEOPENPLAN_P_H
#define DIFFSCOPE_COREPLUGIN_FILEOPENPLAN_P_H

#include <coreplugin/FileOpenPlan.h>

#include <QPointer>
#include <QSharedPointer>

#include <coreplugin/FileOpenDispatcher.h>

namespace Core {

    class FileOpenRequest;

    class FileOpenPlanPrivate {
        Q_DECLARE_PUBLIC(FileOpenPlan)
    public:
        FileOpenPlan *q_ptr = nullptr;
        QPointer<FileOpenDispatcher> dispatcher;
        QSharedPointer<FileOpenRequest> request;
        bool acceptsDrop = false;
        bool invalidated = false;
        bool submitted = false;
    };

}

#endif // DIFFSCOPE_COREPLUGIN_FILEOPENPLAN_P_H
