#ifndef DIFFSCOPE_DSPX_MODEL_MAPDATA_P_H
#define DIFFSCOPE_DSPX_MODEL_MAPDATA_P_H

#include <QHash>
#include <QJSValue>
#include <QStringList>

#include <dspxmodel/private/Model_p.h>

namespace dspx {

    class MapJSIterable {
    public:
        static QJSValue create(QObject *o);
    };

    template <class MapType, class ItemType>
    class MapData {
    public:
        MapType *q_ptr;
        ModelPrivate *pModel;
        QHash<QString, ItemType *> itemMap;
        QJSValue iterable_;

        ItemType *getItem(Handle handle, bool create) const {
            if (auto object = pModel->mapToObject(handle)) {
                auto item = qobject_cast<ItemType *>(object);
                Q_ASSERT(item);
                return item;
            }
            if (create) {
                return pModel->createObject<ItemType>(handle);
            }
            Q_UNREACHABLE();
        }

        void init(const QList<QPair<QString, Handle>> &handles) {
            for (auto &[key, handle] : handles) {
                itemMap.insert(key, getItem(handle, true));
            }
        }

        void insertItem(const QString &key, ItemType *item) {
            auto q = q_ptr;
            Q_EMIT q->itemAboutToInsert(key, item);
            itemMap.insert(key, item);
            Q_EMIT q->itemInserted(key, item);
            Q_EMIT q->sizeChanged(static_cast<int>(itemMap.size()));
            Q_EMIT q->keysChanged();
            Q_EMIT q->itemsChanged();
        }

        void removeItem(const QString &key) {
            auto q = q_ptr;
            Q_EMIT q->itemAboutToRemove(key, itemMap.value(key));
            itemMap.remove(key);
            Q_EMIT q->itemRemoved(key, itemMap.value(key));
            Q_EMIT q->sizeChanged(static_cast<int>(itemMap.size()));
            Q_EMIT q->keysChanged();
        }

        int size() const {
            return static_cast<int>(itemMap.size());
        }

        QStringList keys() const {
            return itemMap.keys();
        }

        QList<ItemType *> items() const {
            return itemMap.values();
        }

        ItemType *item(const QString &key) const {
            return itemMap.value(key);
        }

        bool contains(const QString &key) const {
            return itemMap.contains(key);
        }

        void handleInsertIntoMapContainer(Handle entity, const QString &key) {
            auto q = q_ptr;
            auto item = getItem(entity, true);
            insertItem(key, item);
        }

        void handleTakeFromMapContainer(Handle takenEntity, const QString &key) {
            auto q = q_ptr;
            removeItem(key);
        }

        QJSValue iterable() {
            if (!iterable_.isUndefined()) {
                return iterable_;
            }
            auto q = q_ptr;
            iterable_ = MapJSIterable::create(q);
            return iterable_;
        }
    };

}

#endif //DIFFSCOPE_DSPX_MODEL_MAPDATA_P_H
