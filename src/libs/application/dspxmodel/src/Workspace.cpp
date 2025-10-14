#include "Workspace.h"

#include <QHash>

#include <dspxmodel/private/Model_p.h>
#include <dspxmodel/WorkspaceInfo.h>

namespace dspx {

    class WorkspacePrivate {
        Q_DECLARE_PUBLIC(Workspace)
    public:
        Workspace *q_ptr;
        ModelPrivate *pModel;

        QHash<QString, WorkspaceInfo *> itemMap;

        WorkspaceInfo *getItem(Handle handle, bool create) const;

        void insertItem(const QString &key, WorkspaceInfo *item);
        void removeItem(const QString &key);
    };

    WorkspaceInfo *WorkspacePrivate::getItem(Handle handle, bool create) const {
        if (auto object = pModel->mapToObject(handle)) {
            auto item = qobject_cast<WorkspaceInfo *>(object);
            Q_ASSERT(item);
            return item;
        }
        if (create) {
            return pModel->createObject<WorkspaceInfo>(handle);
        }
        Q_UNREACHABLE();
    }

    void WorkspacePrivate::insertItem(const QString &key, WorkspaceInfo *item) {
        Q_Q(Workspace);
        itemMap.insert(key, item);
        // TODO
    }

    void WorkspacePrivate::removeItem(const QString &key) {
        Q_Q(Workspace);
        itemMap.remove(key);
        // TODO
    }

    Workspace::Workspace(Handle handle, Model *model) : EntityObject(handle, model), d_ptr(new WorkspacePrivate) {
        Q_D(Workspace);
        d->q_ptr = this;
        d->pModel = ModelPrivate::get(model);
    }

    Workspace::~Workspace() = default;

    void Workspace::handleInsertIntoMapContainer(Handle entity, const QString &key) {
        Q_D(Workspace);
        auto workspaceInfo = d->getItem(entity, true);
        d->insertItem(key, workspaceInfo);
    }

    void Workspace::handleTakeFromMapContainer(Handle takenEntity, const QString &key) {
        Q_D(Workspace);
        d->removeItem(key);
    }

}

#include "moc_Workspace.cpp"