#ifndef DIFFSCOPE_DSPX_MODEL_RANGESEQUENCECONTAINER_P_H
#define DIFFSCOPE_DSPX_MODEL_RANGESEQUENCECONTAINER_P_H

#include <QHash>
#include <QList>
#include <QSet>

#include <interval-tree/interval_tree.hpp>

namespace dspx {

    template <class T>
    struct RangeSequenceContainer {
        struct Interval : lib_interval_tree::interval<int> {
            constexpr Interval() : interval(0, 0), m_item(nullptr) {
            }
            constexpr Interval(int position, int length, T *item) : lib_interval_tree::interval<int>(position, position + std::max(length, 1) - 1), m_item(item) {
            }
            constexpr T *item() const {
                return m_item;
            }
            constexpr bool operator==(const Interval &other) const {
                return m_item == other.m_item;
            }

        private:
            T *m_item;
        };
        QHash<T *, QSet<T *>> m_overlapList;
        QHash<T *, Interval> m_intervals;
        lib_interval_tree::interval_tree<Interval> m_intervalTree;

        QList<T *> insertItem(T *item, int position, int length) {
            QSet<T *> affectedItems;
            if (m_intervals.contains(item)) {
                m_intervalTree.erase(m_intervalTree.find(m_intervals.value(item)));
                for (auto overlappedItem : m_overlapList.value(item)) {
                    affectedItems.insert(overlappedItem);
                    m_overlapList[overlappedItem].remove(item);
                }
                m_overlapList[item].clear();
            }
            Interval interval(position, length, item);
            m_overlapList.insert(item, {});
            m_intervalTree.overlap_find_all(interval, [&](const auto &it) {
                auto overlappedItem = it.interval().item();
                if (affectedItems.contains(overlappedItem)) {
                    affectedItems.remove(overlappedItem);
                } else {
                    affectedItems.insert(overlappedItem);
                }
                m_overlapList[item].insert(overlappedItem);
                m_overlapList[overlappedItem].insert(item);
                return true;
            });
            m_intervalTree.insert(interval);
            m_intervals.insert(item, interval);
            affectedItems.insert(item);
            return QList<T *>(affectedItems.cbegin(), affectedItems.cend());
        }

        QList<T *> removeItem(T *item) {
            QSet<T *> affectedItems;
            if (m_intervals.contains(item)) {
                m_intervalTree.erase(m_intervalTree.find(m_intervals.value(item)));
                for (auto overlappedItem : m_overlapList.value(item)) {
                    affectedItems.insert(overlappedItem);
                    m_overlapList[overlappedItem].remove(item);
                }
                m_overlapList.remove(item);
                m_intervals.remove(item);
            }
            return QList<T *>(affectedItems.cbegin(), affectedItems.cend());
        }

        bool isOverlapped(T *item) const {
            return !m_overlapList.value(item).isEmpty();
        }

        QList<T *> slice(int position, int length) const {
            QList<T *> result;
            Interval interval(position, length, nullptr);
            m_intervalTree.overlap_find_all(interval, [&](const auto &it) {
                auto overlappedItem = it.interval().item();
                result.append(overlappedItem);
                return true;
            });
            return result;
        }
    };

}

#endif //DIFFSCOPE_DSPX_MODEL_RANGESEQUENCECONTAINER_P_H
