#include "ModelStrategy.h"
#include "SourceMap.h"

#include <opendspx/sources.h>

#include <dspxmodel/Source.h>
#include <dspxmodel/private/MapData_p.h>
#include <dspxmodel/private/Model_p.h>

namespace dspx {

    class SourceMapPrivate : public MapData<SourceMap, Source> {
        Q_DECLARE_PUBLIC(SourceMap)
    };

    SourceMap::SourceMap(Handle handle, Model *model) : EntityObject(handle, model), d_ptr(new SourceMapPrivate) {
        Q_D(SourceMap);
        Q_ASSERT(model->strategy()->getEntityType(handle) == ModelStrategy::EM_Sources);
        d->q_ptr = this;
        d->pModel = ModelPrivate::get(model);

        d->init(model->strategy()->getEntitiesFromMapContainer(handle));
    }

    SourceMap::~SourceMap() = default;

    int SourceMap::size() const {
        Q_D(const SourceMap);
        return d->size();
    }

    QStringList SourceMap::keys() const {
        Q_D(const SourceMap);
        return d->keys();
    }

    QList<Source *> SourceMap::items() const {
        Q_D(const SourceMap);
        return d->items();
    }

    bool SourceMap::insertItem(const QString &key, Source *item) {
        Q_D(SourceMap);
        return d->pModel->strategy->insertIntoMapContainer(handle(), item->handle(), key);
    }

    Source *SourceMap::removeItem(const QString &key) {
        Q_D(SourceMap);
        auto takenEntityHandle = d->pModel->strategy->takeFromMapContainer(handle(), key);
        return d->getItem(takenEntityHandle, false);
    }

    Source *SourceMap::item(const QString &key) const {
        Q_D(const SourceMap);
        return d->item(key);
    }

    bool SourceMap::contains(const QString &key) const {
        Q_D(const SourceMap);
        return d->contains(key);
    }

    QDspx::Sources SourceMap::toQDspx() const {
        Q_D(const SourceMap);
        QDspx::Sources ret;
        for (const auto &[key, value] : d->itemMap.asKeyValueRange()) {
            ret.insert(key, value->jsonObject());
        }
        return ret;
    }

    void SourceMap::fromQDspx(const QDspx::Sources &sourceMap) {
        for (const auto &key : keys()) {
            removeItem(key);
        }
        for (const auto &[key, value] : sourceMap.asKeyValueRange()) {
            auto source = model()->createSource();
            source->setJsonObject(value);
            insertItem(key, source);
        }
    }

    void SourceMap::handleInsertIntoMapContainer(Handle entity, const QString &key) {
        Q_D(SourceMap);
        d->handleInsertIntoMapContainer(entity, key);
    }

    void SourceMap::handleTakeFromMapContainer(Handle takenEntity, const QString &key) {
        Q_D(SourceMap);
        d->handleTakeFromMapContainer(takenEntity, key);
    }

}

#include "moc_SourceMap.cpp"
