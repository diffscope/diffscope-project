// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_DSPXPROJECT_DOCUMENTPERSISTENCEHOOKREGISTRY_P_H
#define DIFFSCOPE_DSPXPROJECT_DOCUMENTPERSISTENCEHOOKREGISTRY_P_H

#include <dspxproject/DocumentPersistenceHookRegistry.h>

#include <QList>

namespace DspxProject {

    class DocumentPersistenceHookRegistryPrivate {
        Q_DECLARE_PUBLIC(DocumentPersistenceHookRegistry)
    public:
        DocumentPersistenceHookRegistry *q_ptr = nullptr;
        QList<IDocumentPersistenceHook *> hooks;
    };

}

#endif // DIFFSCOPE_DSPXPROJECT_DOCUMENTPERSISTENCEHOOKREGISTRY_P_H
