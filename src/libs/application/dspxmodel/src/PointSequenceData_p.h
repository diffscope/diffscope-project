#ifndef DIFFSCOPE_DSPX_MODEL_POINTSEQUENCEDATA_P_H
#define DIFFSCOPE_DSPX_MODEL_POINTSEQUENCEDATA_P_H

#include <QJSValue>

#include <dspxmodel/private/Model_p.h>
#include <dspxmodel/private/PointSequenceContainer_p.h>

namespace dspx {

    class PointSequenceJSIterable {
    public:
        static QJSValue create(QObject *o);
    };

    template <class SequenceType, class ItemType, int (ItemType::*positionGetter)() const, void (ItemType::*positionChangedSignal)(int)>
    class PointSequenceData {
    public:
        SequenceType *q_ptr;
        ModelPrivate *pModel;
        PointSequenceContainer<ItemType> container;
        ItemType *firstItem{};
        ItemType *lastItem{};
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

        void insertItem(ItemType *item, int position) {
            auto q = q_ptr;
            bool containsItem = container.contains(item);
            if (!containsItem) {
                Q_EMIT q->itemAboutToInsert(item);
            }
            container.insertItem(item, position);
            if (!containsItem) {
                updateFirstAndLastItem();
                Q_EMIT q->itemInserted(item);
                Q_EMIT q->sizeChanged(container.size());
            }
        }

        void removeItem(ItemType *item) {
            auto q = q_ptr;
            Q_EMIT q->itemAboutToRemove(item);
            container.removeItem(item);
            updateFirstAndLastItem();
            Q_EMIT q->itemRemoved(item);
            Q_EMIT q->sizeChanged(container.size());
        }
        void updateFirstAndLastItem() {
            auto q = q_ptr;
            if (auto a = container.firstItem(); firstItem != a) {
                firstItem = a;
                Q_EMIT q->firstItemChanged(firstItem);
            }
            if (auto a = container.lastItem(); lastItem != a) {
                lastItem = a;
                Q_EMIT q->lastItemChanged(lastItem);
            }
        }

        void handleInsertIntoSequenceContainer(Handle entity) {
            auto q = q_ptr;
            auto item = getItem(entity, true);
            QObject::connect(item, positionChangedSignal, q, [=](int pos) {
                insertItem(item, pos);
            });
            QObject::connect(item, &QObject::destroyed, q, [=] {
                removeItem(item);
            });
            insertItem(item, (item->*positionGetter)());
        }

        void handleTakeFromSequenceContainer(Handle takenEntity, Handle entity) {
            auto q = q_ptr;
            auto item = getItem(takenEntity, false);
            QObject::disconnect(item, nullptr, q, nullptr);
            removeItem(item);
        }

        QJSValue iterable() {
            if (!iterable_.isUndefined()) {
                return iterable_;
            }
            auto q = q_ptr;
            iterable_ = PointSequenceJSIterable::create(q);
            return iterable_;
        }
    };

}

#endif //DIFFSCOPE_DSPX_MODEL_POINTSEQUENCEDATA_P_H
