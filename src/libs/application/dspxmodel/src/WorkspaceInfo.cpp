#include "WorkspaceInfo.h"

#include <QVariant>
#include <QJsonObject>

#include <dspxmodel/Model.h>
#include <dspxmodel/ModelStrategy.h>

namespace dspx {

    class WorkspaceInfoPrivate {
        Q_DECLARE_PUBLIC(WorkspaceInfo)
    public:
        WorkspaceInfo *q_ptr;
        QJsonObject jsonObject;
    };

    WorkspaceInfo::WorkspaceInfo(Handle handle, Model *model) : EntityObject(handle, model), d_ptr(new WorkspaceInfoPrivate) {
        Q_D(WorkspaceInfo);
        Q_ASSERT(model->strategy()->getEntityType(handle) == ModelStrategy::EI_WorkspaceInfo);
        d->q_ptr = this;
        d->jsonObject = model->strategy()->getEntityProperty(handle, ModelStrategy::P_JsonObject).toJsonObject();
    }

    WorkspaceInfo::~WorkspaceInfo() = default;

    QJsonObject WorkspaceInfo::jsonObject() const {
        Q_D(const WorkspaceInfo);
        return d->jsonObject;
    }

    void WorkspaceInfo::setJsonObject(const QJsonObject &jsonObject) {
        Q_D(WorkspaceInfo);
        model()->strategy()->setEntityProperty(handle(), ModelStrategy::P_JsonObject, QVariant::fromValue(jsonObject));
    }

    void WorkspaceInfo::handleSetEntityProperty(int property, const QVariant &value) {
        Q_D(WorkspaceInfo);
        switch (property) {
            case ModelStrategy::P_JsonObject: {
                d->jsonObject = value.toJsonObject();
                Q_EMIT jsonObjectChanged(d->jsonObject);
                break;
            }
            default:
                Q_UNREACHABLE();
        }
    }

}

#include "moc_WorkspaceInfo.cpp"