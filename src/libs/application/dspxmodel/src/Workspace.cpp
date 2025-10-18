#include "Workspace.h"

#include "ModelStrategy.h"

#include <opendspx/qdspxmodel.h>

#include <dspxmodel/private/Model_p.h>
#include <dspxmodel/private/MapData_p.h>
#include <dspxmodel/WorkspaceInfo.h>

namespace dspx {

    class WorkspacePrivate : public MapData<Workspace, WorkspaceInfo> {
        Q_DECLARE_PUBLIC(Workspace)
    };
    

    Workspace::Workspace(Handle handle, Model *model) : EntityObject(handle, model), d_ptr(new WorkspacePrivate) {
        Q_D(Workspace);
        Q_ASSERT(model->strategy()->getEntityType(handle) == ModelStrategy::EM_Workspace);
        d->q_ptr = this;
        d->pModel = ModelPrivate::get(model);
    }

    Workspace::~Workspace() = default;

    int Workspace::size() const {
        Q_D(const Workspace);
        return d->size();
    }

    QStringList Workspace::keys() const {
        Q_D(const Workspace);
        return d->keys();
    }

    QList<WorkspaceInfo *> Workspace::items() const {
        Q_D(const Workspace);
        return d->items();
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
        return d->item(key);
    }

    bool Workspace::contains(const QString &key) const {
        Q_D(const Workspace);
        return d->contains(key);
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
        d->handleInsertIntoMapContainer(entity, key);
    }

    void Workspace::handleTakeFromMapContainer(Handle takenEntity, const QString &key) {
        Q_D(Workspace);
        d->handleTakeFromMapContainer(takenEntity, key);
    }

}

#include "moc_Workspace.cpp"