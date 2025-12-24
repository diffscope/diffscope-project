#ifndef DIFFSCOPE_DSPX_MODEL_GENERICGLOBALITEMSELECTIONMODELDATA_P_H
#define DIFFSCOPE_DSPX_MODEL_GENERICGLOBALITEMSELECTIONMODELDATA_P_H

#include <QSet>

#include <dspxmodel/SelectionModel.h>

namespace dspx {

    template <
        class PublicClass,
        class PrivateClass,
        class Item,
        void (Item::*superItemChangedSignal)(),
        class SuperItem = void
    >
    class GenericGlobalItemSelectionModelData {
    public:
        PublicClass *q_ptr;
        SelectionModel *selectionModel;
        Item *currentItem = nullptr;
        QSet<Item *> selectedItems;
        SuperItem *superItem = nullptr;

        void select(Item *item, SelectionModel::SelectionCommand command) {
            auto q = q_ptr;
            bool selectionUpdatedFlag = false;
            bool currentItemUpdatedFlag = false;

            if (item && !static_cast<PrivateClass *>(this)->isAddedToModel(item)) {
                return;
            }

            if (auto currentSuperItem = static_cast<PrivateClass *>(this)->getSuperItem(item); superItem && currentSuperItem && superItem != currentSuperItem) {
                selectionUpdatedFlag = true;
                currentItemUpdatedFlag = true;
                clearSelection();
                setCurrentItem(nullptr);
                static_cast<PrivateClass *>(this)->clearSuperItem();
            }

            if ((command & SelectionModel::ClearPreviousSelection)) {
                if (!selectedItems.isEmpty()) {
                    selectionUpdatedFlag = true;
                }
                clearSelection();
            }

            if (command & SelectionModel::SetCurrentItem) {
                if (currentItem != item) {
                    currentItemUpdatedFlag = true;
                    setCurrentItem(item);
                }
            }

            // Update selection
            if (item) {
                if ((command & SelectionModel::Select) && (command & SelectionModel::Deselect)) {
                    // Toggle
                    selectionUpdatedFlag = true;
                    if (selectedItems.contains(item)) {
                        removeFromSelection(item);
                    } else {
                        addToSelection(item);
                    }
                } else if (command & SelectionModel::Select) {
                    // Select only
                    if (!selectedItems.contains(item)) {
                        selectionUpdatedFlag = true;
                        addToSelection(item);
                    }
                } else if (command & SelectionModel::Deselect) {
                    // Deselect only
                    if (selectedItems.contains(item)) {
                        selectionUpdatedFlag = true;
                        removeFromSelection(item);
                    }
                }
            }

            // Emit signals
            if (selectionUpdatedFlag) {
                Q_EMIT q->selectedItemsChanged();
                Q_EMIT q->selectedCountChanged();
            }
            if (currentItemUpdatedFlag) {
                Q_EMIT q->currentItemChanged();
            }
        }

        void setCurrentItem(Item *item) {
            auto q = q_ptr;
            if (currentItem) {
                if (!selectedItems.contains(currentItem)) {
                    QObject::disconnect(currentItem, nullptr, q, nullptr);
                }
            }
            currentItem = item;
            static_cast<PrivateClass *>(this)->updateSuperItem(getSuperItem(item));
            if (item) {
                if (!selectedItems.contains(item)) {
                    QObject::connect(item, superItemChangedSignal, q, [item, this] {
                        if (static_cast<PrivateClass *>(this)->isAddedToModel(item)) {
                            static_cast<PrivateClass *>(this)->updateAssociation(item);
                        } else {
                            updateOnItemRemoved(item);
                        }
                    });
                    QObject::connect(item, &QObject::destroyed, q, [item, this] {
                        updateOnItemRemoved(item);
                    });
                }
            }
        }

        void addToSelection(Item *item) {
            auto q = q_ptr;
            if (currentItem != item) {
                QObject::connect(item, superItemChangedSignal, q, [item, this] {
                    if (static_cast<PrivateClass *>(this)->isAddedToModel(item)) {
                        static_cast<PrivateClass *>(this)->updateAssociation(item);
                    } else {
                        updateOnItemRemoved(item);
                    }
                });
                QObject::connect(item, &QObject::destroyed, q, [item, this] {
                    updateOnItemRemoved(item);
                });
            }
            selectedItems.insert(item);
            static_cast<PrivateClass *>(this)->updateSuperItem(getSuperItem(item));
            static_cast<PrivateClass *>(this)->updateAssociation(item);
            Q_EMIT q->itemSelected(item, true);
        }
        void removeFromSelection(Item *item) {
            auto q = q_ptr;
            if (currentItem != item) {
                QObject::disconnect(item, nullptr, q, nullptr);
            }
            selectedItems.remove(item);
            static_cast<PrivateClass *>(this)->removeAssociation(item);
            Q_EMIT q->itemSelected(item, false);
        }
        void clearSelection() {
            auto q = q_ptr;
            for (auto item : selectedItems) {
                if (currentItem != item) {
                    QObject::disconnect(item, nullptr, q, nullptr);
                }
                Q_EMIT q->itemSelected(item, false);
            }
            selectedItems.clear();
            static_cast<PrivateClass *>(this)->clearAssociation();
        }
        void updateOnItemRemoved(Item *item) {
            auto q = q_ptr;
            bool selectionUpdatedFlag = false;
            bool currentItemUpdatedFlag = false;
            if (currentItem == item) {
                currentItemUpdatedFlag = true;
                setCurrentItem(nullptr);
            }
            if (selectedItems.contains(item)) {
                selectionUpdatedFlag = true;
                removeFromSelection(item);
            }
            if (selectionUpdatedFlag) {
                Q_EMIT q->selectedItemsChanged();
                Q_EMIT q->selectedCountChanged();
            }
            if (currentItemUpdatedFlag) {
                Q_EMIT q->currentItemChanged();
            }
        }

        void clearAll() {
            auto q = q_ptr;
            clearSelection();
            setCurrentItem(nullptr);
            static_cast<PrivateClass *>(this)->clearSuperItem();
            Q_EMIT q->selectedItemsChanged();
            Q_EMIT q->selectedCountChanged();
            Q_EMIT q->currentItemChanged();
        }

        static void updateAssociation(Item *) {}
        static void removeAssociation(Item *) {}
        static void clearAssociation() {}
        static SuperItem *getSuperItem(Item *) { return nullptr; }
        void clearSuperItem() { superItem = nullptr; }
        void updateSuperItem(SuperItem *superItem_) { superItem = superItem_; }

    };

}

#endif //DIFFSCOPE_DSPX_MODEL_GENERICGLOBALITEMSELECTIONMODELDATA_P_H
