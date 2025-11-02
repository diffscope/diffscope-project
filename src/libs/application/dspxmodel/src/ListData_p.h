#ifndef DIFFSCOPE_DSPX_MODEL_LISTDATA_P_H
#define DIFFSCOPE_DSPX_MODEL_LISTDATA_P_H

#include <algorithm>

#include <QJSValue>
#include <QList>

#include <dspxmodel/private/Model_p.h>

namespace dspx {

    class ListJSIterable {
    public:
        static QJSValue create(QObject *o);
    };

    template <class ListType, class ItemType>
    class ListData {
    public:
        ListType *q_ptr;
        ModelPrivate *pModel;
        QList<ItemType *> itemList;
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

        void insertItem(int index, ItemType *item) {
            auto q = q_ptr;
            Q_EMIT q->itemAboutToInsert(index, item);
            itemList.insert(index, item);
            Q_EMIT q->itemInserted(index, item);
            Q_EMIT q->sizeChanged(itemList.size());
            Q_EMIT q->itemsChanged();
        }

        void removeItem(int index) {
            auto q = q_ptr;
            auto item = itemList.at(index);
            Q_EMIT q->itemAboutToRemove(index, item);
            itemList.removeAt(index);
            Q_EMIT q->itemRemoved(index, item);
            Q_EMIT q->sizeChanged(itemList.size());
            Q_EMIT q->itemsChanged();
        }

        int size() const {
            return itemList.size();
        }

        QList<ItemType *> items() const {
            return itemList;
        }

        ItemType *item(int index) const {
            if (index < 0 || index >= itemList.size()) {
                return nullptr;
            }
            return itemList.at(index);
        }

        void handleInsertIntoListContainer(Handle entity, int index) {
            auto q = q_ptr;
            auto item = getItem(entity, true);
            insertItem(index, item);
        }

        void handleTakeFromListContainer(Handle takenEntity, int index) {
            auto q = q_ptr;
            removeItem(index);
        }

        void handleRotateListContainer(int leftIndex, int middleIndex, int rightIndex) {
            auto q = q_ptr;
            Q_EMIT q->aboutToRotate(leftIndex, middleIndex, rightIndex);

            std::rotate(itemList.begin() + leftIndex, itemList.begin() + middleIndex, itemList.begin() + rightIndex);

            Q_EMIT q->rotated(leftIndex, middleIndex, rightIndex);
            Q_EMIT q->itemsChanged();
        }

        QJSValue iterable() {
            if (!iterable_.isUndefined()) {
                return iterable_;
            }
            auto q = q_ptr;
            iterable_ = ListJSIterable::create(q);
            return iterable_;
        }
    };

}

#endif //DIFFSCOPE_DSPX_MODEL_LISTDATA_P_H
