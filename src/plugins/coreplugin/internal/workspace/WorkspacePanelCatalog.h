// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#ifndef DIFFSCOPE_COREPLUGIN_WORKSPACEPANELCATALOG_H
#define DIFFSCOPE_COREPLUGIN_WORKSPACEPANELCATALOG_H

#include <QObject>
#include <QStringList>
#include <QVariantList>

namespace Core::Internal {
    class WorkspacePanelCatalog : public QObject {
        Q_OBJECT
    public:
        explicit WorkspacePanelCatalog(QObject *parent = nullptr);
        void capture();
        QVariantList entries() const;
        QVariantList pendingEntries() const;
        void markPresented();

    private:
        QStringList m_ids;
        QStringList m_added;
        bool m_presented = false;
    };
}

#endif

