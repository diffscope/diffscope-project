#include "Workspace.h"

#include "ModelStrategy.h"

#include <QHash>

#include <opendspx/qdspxmodel.h>

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
        auto previousValue = itemMap.value(key);
        if (previousValue == item) {
            return;
        }
        if (previousValue) {
            Q_EMIT q->itemAboutToRemove(key, previousValue);
            itemMap.remove(key);
            Q_EMIT q->itemRemoved(key, previousValue);
        }
        Q_EMIT q->itemAboutToInsert(key, item);
        itemMap.insert(key, item);
        Q_EMIT q->itemInserted(key, item);
        if (!previousValue) {
            Q_EMIT q->sizeChanged(static_cast<int>(itemMap.size()));
            Q_EMIT q->keysChanged();
        }
        Q_EMIT q->itemsChanged();
    }

    void WorkspacePrivate::removeItem(const QString &key) {
        Q_Q(Workspace);
        Q_EMIT q->itemAboutToRemove(key, itemMap.value(key));
        itemMap.remove(key);
        Q_EMIT q->itemRemoved(key, itemMap.value(key));
        Q_EMIT q->sizeChanged(static_cast<int>(itemMap.size()));
        Q_EMIT q->keysChanged();
    }

    Workspace::Workspace(Handle handle, Model *model) : EntityObject(handle, model), d_ptr(new WorkspacePrivate) {
        Q_D(Workspace);
        d->q_ptr = this;
        d->pModel = ModelPrivate::get(model);
    }

    Workspace::~Workspace() = default;

    int Workspace::size() const {
        Q_D(const Workspace);
        return static_cast<int>(d->itemMap.size());
    }

    QStringList Workspace::keys() const {
        Q_D(const Workspace);
        return d->itemMap.keys();
    }

    QList<WorkspaceInfo *> Workspace::items() const {
        Q_D(const Workspace);
        return d->itemMap.values();
    }

    bool Workspace::insertItem(const QString &key, WorkspaceInfo *item) {
        Q_D(Workspace);
        return d->pModel->strategy->insertIntoMapContainer(handle(), item->handle(), key);
    }

    WorkspaceInfo *Workspace::removeItem(const QString &key) {
        Q_D(Workspace);
        auto takenEntityHandle = d->pModel->strategy->takeFromMapContainer(handle(), key);
        return d->getItem(takenEntityHandle, false);
    }

    WorkspaceInfo *Workspace::item(const QString &key) const {
        Q_D(const Workspace);
        return d->itemMap.value(key);
    }

    bool Workspace::contains(const QString &key) const {
        Q_D(const Workspace);
        return d->itemMap.contains(key);
    }

    QDspx::Workspace Workspace::toQDspx() const {
        Q_D(const Workspace);
        QDspx::Workspace ret;
        for (const auto &[key, value] : d->itemMap.asKeyValueRange()) {
            ret.insert(key, value->jsonObject());
        }
        return ret;
    }

    void Workspace::fromQDspx(const QDspx::Workspace &workspace) {
        for (const auto &key : keys()) {
            removeItem(key);
        }
        for (const auto &[key, value] : workspace.asKeyValueRange()) {
            auto workspaceInfo = model()->createWorkspaceInfo();
            workspaceInfo->setJsonObject(value);
            insertItem(key, workspaceInfo);
        }
    }

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