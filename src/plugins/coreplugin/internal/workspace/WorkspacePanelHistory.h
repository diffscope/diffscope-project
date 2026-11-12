// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#ifndef DIFFSCOPE_COREPLUGIN_WORKSPACEPANELHISTORY_H
#define DIFFSCOPE_COREPLUGIN_WORKSPACEPANELHISTORY_H

#include <QObject>
#include <QSet>
#include <QString>
#include <QStringList>

namespace Core::Internal {
    class WorkspacePanelHistory : public QObject {
        Q_OBJECT
    public:
        explicit WorkspacePanelHistory(QObject *parent = nullptr);
        void observe(const QStringList &ids);
        bool isPending(const QString &id) const;
        void markPresented(const QStringList &ids);

    Q_SIGNALS:
        void presentedChanged();

    private:
        QSet<QString> m_previousIds;
        QSet<QString> m_observedIds;
        QSet<QString> m_presentedIds;
        bool m_hasPreviousSnapshot;
        bool m_hasObservedWindow = false;
    };
}

#endif
