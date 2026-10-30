// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_BATCHPROCESS_DOCUMENTINTERFACE_P_H
#define DIFFSCOPE_BATCHPROCESS_DOCUMENTINTERFACE_P_H

#include <batchprocess/DocumentInterface.h>

#include <QHash>
#include <QPointer>
#include <QString>

namespace BatchProcess {
    class Storage;

    class DocumentInterfacePrivate {
        Q_DECLARE_PUBLIC(DocumentInterface)
    public:
        explicit DocumentInterfacePrivate(DocumentInterface *q) : q_ptr(q) {}
        DocumentInterface *q_ptr;
        Internal::CoreModuleExtension *extension{};
        QHash<dspx::Model *, QPointer<Core::DspxDocument>> documents;
        QHash<dspx::Model *, quint64> generations;
        QHash<Core::DspxDocument *, QHash<QString, QPointer<Storage>>> storages;
        quint64 nextGeneration{};
    };
}

#endif // DIFFSCOPE_BATCHPROCESS_DOCUMENTINTERFACE_P_H
