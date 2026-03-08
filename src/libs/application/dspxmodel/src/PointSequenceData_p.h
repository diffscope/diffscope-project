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

    template <
        class SequenceType,
        class ItemType,
        int (ItemType::*positionGetter)() const,
        void (ItemType::*positionChangedSignal)(int),
        void (*setSequence)(ItemType *item, SequenceType *sequence),
        void (*setPreviousItem)(ItemType *item, ItemType *previousItem),
        void (*setNextItem)(ItemType *item, ItemType *nextItem)>
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

        void init(const QList<Handle> &handles) {
            auto q = q_ptr;
            for (auto handle : handles) {
                auto item = getItem(handle, true);
                container.insertItem(item, (item->*positionGetter)());
                setSequence(item, q);
            }
            updateFirstAndLastItem();
        }

        void insertItem(ItemType *item, int position) {
            auto q = q_ptr;
            bool containsItem = container.contains(item);
            auto oldPreviousItem = container.previousItem(item);
            auto oldNextItem = container.nextItem(item);
            if (!containsItem) {
                Q_EMIT q->itemAboutToInsert(item);
            }
            container.insertItem(item, position);
            setSequence(item, q);
            updateFirstAndLastItem();
            auto newPreviousItem = container.previousItem(item);
            auto newNextItem = container.nextItem(item);
            if (oldPreviousItem) {
                setNextItem(oldPreviousItem, oldNextItem);
            }
            if (oldNextItem) {
                setPreviousItem(oldNextItem, oldPreviousItem);
            }
            if (newPreviousItem) {
                setNextItem(newPreviousItem, item);
            }
            if (newNextItem) {
                setPreviousItem(newNextItem, item);
            }
            setPreviousItem(item, newPreviousItem);
            setNextItem(item, newNextItem);
            if (!containsItem) {
                Q_EMIT q->itemInserted(item);
                Q_EMIT q->sizeChanged(container.size());
            }
        }

        void removeItem(ItemType *item) {
            auto q = q_ptr;
            Q_EMIT q->itemAboutToRemove(item);
            auto oldPreviousItem = container.previousItem(item);
            auto oldNextItem = container.nextItem(item);
            container.removeItem(item);
            setSequence(item, nullptr);
            updateFirstAndLastItem();
            if (oldPreviousItem) {
                setNextItem(oldPreviousItem, oldNextItem);
            }
            if (oldNextItem) {
                setPreviousItem(oldNextItem, oldPreviousItem);
            }
            setPreviousItem(item, nullptr);
            setNextItem(item, nullptr);
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
            if (!container.contains(item)) {
                QObject::connect(item, positionChangedSignal, q, [=](int pos) {
                    insertItem(item, pos);
                });
                QObject::connect(item, &QObject::destroyed, q, [=] {
                    removeItem(item);
                });
            }
            insertItem(item, (item->*positionGetter)());
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

#endif //DIFFSCOPE_DSPX_MODEL_POINTSEQUENCEDATA_P_H
