// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#ifndef DIFFSCOPE_COREPLUGIN_WORKSPACEPANELREGISTRY_P_H
#define DIFFSCOPE_COREPLUGIN_WORKSPACEPANELREGISTRY_P_H

#include <coreplugin/WorkspacePanelRegistry.h>

#include <QHash>
#include <QMetaObject>
#include <QPointer>

namespace Core {

    class WorkspacePanelRegistryPrivate {
        Q_DECLARE_PUBLIC(WorkspacePanelRegistry)
    public:
        struct Entry {
            QPointer<QQmlComponent> component;
            WorkspacePanelRegistry::Metadata metadata;
            QMetaObject::Connection destroyedConnection;
            QMetaObject::Connection statusConnection;
        };

        WorkspacePanelRegistry *q_ptr;
        QHash<QString, Entry> entries;
        QStringList ids;
    };

}

#endif
