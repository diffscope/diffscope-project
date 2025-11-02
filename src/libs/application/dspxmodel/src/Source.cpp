#include "Source.h"

#include <QJsonObject>
#include <QVariant>

#include <dspxmodel/Model.h>
#include <dspxmodel/ModelStrategy.h>

namespace dspx {

    class SourcePrivate {
        Q_DECLARE_PUBLIC(Source)
    public:
        Source *q_ptr;
        QJsonObject jsonObject;
    };

    Source::Source(Handle handle, Model *model) : EntityObject(handle, model), d_ptr(new SourcePrivate) {
        Q_D(Source);
        Q_ASSERT(model->strategy()->getEntityType(handle) == ModelStrategy::EI_Source);
        d->q_ptr = this;
        d->jsonObject = model->strategy()->getEntityProperty(handle, ModelStrategy::P_JsonObject).toJsonObject();
    }

    Source::~Source() = default;

    QJsonObject Source::jsonObject() const {
        Q_D(const Source);
        return d->jsonObject;
    }

    void Source::setJsonObject(const QJsonObject &jsonObject) {
        Q_D(Source);
        model()->strategy()->setEntityProperty(handle(), ModelStrategy::P_JsonObject, QVariant::fromValue(jsonObject));
    }

    void Source::handleSetEntityProperty(int property, const QVariant &value) {
        Q_D(Source);
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

#include "moc_Source.cpp"
