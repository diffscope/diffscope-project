#ifndef DIFFSCOPE_DSPX_MODEL_RANGESEQUENCEDATA_P_H
#define DIFFSCOPE_DSPX_MODEL_RANGESEQUENCEDATA_P_H

#include <QJSValue>

#include <dspxmodel/private/Model_p.h>
#include <dspxmodel/private/PointSequenceData_p.h>
#include <dspxmodel/private/RangeSequenceContainer_p.h>

namespace dspx {

    class PointSequenceJSIterable;

    template <class SequenceType, class ItemType, int (ItemType::*positionGetter)() const, void (ItemType::*positionChangedSignal)(int), int (ItemType::*lengthGetter)() const, void (ItemType::*lengthChangedSignal)(int), void (*setOverlapped)(ItemType *item, bool overlapped)>
    class RangeSequenceData {
    public:
        SequenceType *q_ptr;
        ModelPrivate *pModel;
        PointSequenceContainer<ItemType> pointContainer;
        RangeSequenceContainer<ItemType> rangeContainer;
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

        void init(const QList<Handle> &handles) {
            for (auto handle : handles) {
                auto item = getItem(handle, true);
                pointContainer.insertItem(item, (item->*positionGetter)());
                auto affectedItems = rangeContainer.insertItem(item, (item->*positionGetter)(), (item->*lengthGetter)());
                for (auto affectedItem : affectedItems) {
                    bool isOverlapped = rangeContainer.isOverlapped(affectedItem);
                    setOverlapped(affectedItem, isOverlapped);
                }
            }
            updateFirstAndLastItem();
        }

        void insertItem(ItemType *item, int position, int length) {
            auto q = q_ptr;
            bool containsItem = pointContainer.contains(item);
            if (!containsItem) {
                Q_EMIT q->itemAboutToInsert(item);
            }

            // Insert into both containers
            pointContainer.insertItem(item, position);
            auto affectedItems = rangeContainer.insertItem(item, position, length);

            // Update overlapped status for all affected items
            for (auto affectedItem : affectedItems) {
                bool isOverlapped = rangeContainer.isOverlapped(affectedItem);
                setOverlapped(affectedItem, isOverlapped);
            }

            if (!containsItem) {
                updateFirstAndLastItem();
                Q_EMIT q->itemInserted(item);
                Q_EMIT q->sizeChanged(pointContainer.size());
            }
        }

        void removeItem(ItemType *item) {
            auto q = q_ptr;
            Q_EMIT q->itemAboutToRemove(item);

            // Remove from both containers
            pointContainer.removeItem(item);
            auto affectedItems = rangeContainer.removeItem(item);

            // Update overlapped status for all affected items
            for (auto affectedItem : affectedItems) {
                bool isOverlapped = rangeContainer.isOverlapped(affectedItem);
                setOverlapped(affectedItem, isOverlapped);
            }

            updateFirstAndLastItem();
            Q_EMIT q->itemRemoved(item);
            Q_EMIT q->sizeChanged(pointContainer.size());
        }

        void updateFirstAndLastItem() {
            auto q = q_ptr;
            if (auto a = pointContainer.firstItem(); firstItem != a) {
                firstItem = a;
                Q_EMIT q->firstItemChanged(firstItem);
            }
            if (auto a = pointContainer.lastItem(); lastItem != a) {
                lastItem = a;
                Q_EMIT q->lastItemChanged(lastItem);
            }
        }

        void handleInsertIntoSequenceContainer(Handle entity) {
            auto q = q_ptr;
            auto item = getItem(entity, true);
            if (!pointContainer.contains(item)) {
                QObject::connect(item, positionChangedSignal, q, [=](int pos) {
                    int length = (item->*lengthGetter)();
                    insertItem(item, pos, length);
                });
                QObject::connect(item, lengthChangedSignal, q, [=](int len) {
                    int position = (item->*positionGetter)();
                    insertItem(item, position, len);
                });
                QObject::connect(item, &QObject::destroyed, q, [=] {
                    removeItem(item);
                });
            }
            insertItem(item, (item->*positionGetter)(), (item->*lengthGetter)());
        }

        void handleTakeFromSequenceContainer(Handle takenEntity, Handle entity) {
            auto q = q_ptr;
            auto item = getItem(takenEntity, false);
            if (item) {
                QObject::disconnect(item, nullptr, q, nullptr);
                removeItem(item);
            }
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

#endif //DIFFSCOPE_DSPX_MODEL_RANGESEQUENCEDATA_P_H
