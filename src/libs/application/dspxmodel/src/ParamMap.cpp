#include "ModelStrategy.h"
#include "ParamMap.h"

#include <opendspx/param.h>

#include <dspxmodel/Param.h>
#include <dspxmodel/private/MapData_p.h>
#include <dspxmodel/private/Model_p.h>

namespace dspx {

    class ParamMapPrivate : public MapData<ParamMap, Param> {
        Q_DECLARE_PUBLIC(ParamMap)
    };

    ParamMap::ParamMap(Handle handle, Model *model) : EntityObject(handle, model), d_ptr(new ParamMapPrivate) {
        Q_D(ParamMap);
        Q_ASSERT(model->strategy()->getEntityType(handle) == ModelStrategy::EM_Params);
        d->q_ptr = this;
        d->pModel = ModelPrivate::get(model);
    }

    ParamMap::~ParamMap() = default;

    int ParamMap::size() const {
        Q_D(const ParamMap);
        return d->size();
    }

    QStringList ParamMap::keys() const {
        Q_D(const ParamMap);
        return d->keys();
    }

    QList<Param *> ParamMap::items() const {
        Q_D(const ParamMap);
        return d->items();
    }

    bool ParamMap::insertItem(const QString &key, Param *item) {
        Q_D(ParamMap);
        return d->pModel->strategy->insertIntoMapContainer(handle(), item->handle(), key);
    }

    Param *ParamMap::removeItem(const QString &key) {
        Q_D(ParamMap);
        auto takenEntityHandle = d->pModel->strategy->takeFromMapContainer(handle(), key);
        return d->getItem(takenEntityHandle, false);
    }

    Param *ParamMap::item(const QString &key) const {
        Q_D(const ParamMap);
        return d->item(key);
    }

    bool ParamMap::contains(const QString &key) const {
        Q_D(const ParamMap);
        return d->contains(key);
    }

    QDspx::Params ParamMap::toQDspx() const {
        Q_D(const ParamMap);
        QDspx::Params ret;
        for (const auto &[key, value] : d->itemMap.asKeyValueRange()) {
            ret.insert(key, value->toQDspx());
        }
        return ret;
    }

    void ParamMap::fromQDspx(const QDspx::Params &paramMap) {
        for (const auto &key : keys()) {
            removeItem(key);
        }
        for (const auto &[key, value] : paramMap.asKeyValueRange()) {
            auto param = model()->createParam();
            param->fromQDspx(value);
            insertItem(key, param);
        }
    }

    void ParamMap::handleInsertIntoMapContainer(Handle entity, const QString &key) {
        Q_D(ParamMap);
        d->handleInsertIntoMapContainer(entity, key);
    }

    void ParamMap::handleTakeFromMapContainer(Handle takenEntity, const QString &key) {
        Q_D(ParamMap);
        d->handleTakeFromMapContainer(takenEntity, key);
    }

}

#include "moc_ParamMap.cpp"
