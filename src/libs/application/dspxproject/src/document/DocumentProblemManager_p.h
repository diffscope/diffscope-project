// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_DSPXPROJECT_DOCUMENTPROBLEMMANAGER_P_H
#define DIFFSCOPE_DSPXPROJECT_DOCUMENTPROBLEMMANAGER_P_H

#include <dspxproject/DocumentProblemManager.h>

#include <QHash>
#include <QMap>
#include <QMetaObject>
#include <QPointer>
#include <QSet>
#include <QSharedPointer>

namespace DspxProject {

    class DocumentProblemManagerPrivate {
        Q_DECLARE_PUBLIC(DocumentProblemManager)
    public:
        struct Record {
            DocumentProblem problem;
            QObject *owner;
            QString key;
            dspx::Handle target;
        };

        struct Batch {
            QHash<QString, DocumentProblemId> ids;
            QMetaObject::Connection destroyed;
        };

        struct Target {
            QPointer<dspx::EntityObject> entity;
            QList<dspx::Handle> path;
            QSet<DocumentProblemId> ids;
        };

        struct Watch {
            QPointer<dspx::EntityObject> entity;
            QSet<dspx::Handle> targets;
            QMetaObject::Connection parentChanged;
            QMetaObject::Connection destroyed;
        };

        struct Operation {
            QPointer<QObject> context;
            QString type;
            QString text;
            DocumentProblemManager::Handler handler;
            DocumentProblemManager::Predicate predicate;
            QMetaObject::Connection destroyed;

            bool appliesTo(const DocumentProblem &problem) const;
        };

        dspx::EntityObject *parentEntity(dspx::EntityObject *entity, QMetaObject::Connection *connection = nullptr);
        QList<dspx::EntityObject *> entityPath(dspx::EntityObject *entity);
        bool isLive(const dspx::EntityObject *entity) const;
        bool isAttached(const QList<dspx::Handle> &path) const;
        void updatePath(dspx::Handle handle, Target &target, const QList<dspx::EntityObject *> &entities);
        void markTopologyDirty(dspx::Handle handle);
        void refreshTopology();
        void queueNotification();
        void invalidate(const QList<dspx::Handle> &path);

        bool updateProblems(QObject *owner, const QList<DocumentProblemReport> &reports, QStringList removedKeys, bool replace);
        void addProblem(QObject *owner, const DocumentProblemReport &report, const QList<dspx::EntityObject *> &path);
        void removeProblem(DocumentProblemId id);
        DocumentProblemOperationId registerOperation(QObject *context, const QString &type, const QString &text,
                                                     DocumentProblemManager::Handler handler, DocumentProblemManager::Predicate predicate);

        DocumentProblemManager *q_ptr = nullptr;
        dspx::Model *model = nullptr;
        QHash<DocumentProblemId, Record> records;
        QHash<QObject *, Batch> batches;
        QHash<dspx::Handle, Target> targets;
        QHash<dspx::Handle, Watch> watches;
        QMap<DocumentProblemOperationId, QSharedPointer<Operation>> operations;
        QSet<dspx::Handle> affectedEntities;
        quint64 nextProblemId = 1;
        quint64 nextOperationId = 1;
        QSet<dspx::Handle> dirtyWatches;
        bool notificationQueued = false;
        bool operationsDirty = false;
    };

}

#endif // DIFFSCOPE_DSPXPROJECT_DOCUMENTPROBLEMMANAGER_P_H
