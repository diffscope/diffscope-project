#ifndef DIFFSCOPE_DSPX_MODEL_POINTSEQUENCECONTAINER_P_H
#define DIFFSCOPE_DSPX_MODEL_POINTSEQUENCECONTAINER_P_H

#include <algorithm>
#include <map>

#include <QHash>
#include <QList>

namespace dspx {

    template <class T>
    struct PointSequenceContainer {
        std::multimap<int, T *> m_items;
        QHash<T *, int> m_positions;

        T *firstItem() const {
            if (m_items.empty()) {
                return nullptr;
            }
            return m_items.cbegin()->second;
        }

        T *lastItem() const {
            if (m_items.empty()) {
                return nullptr;
            }
            return m_items.crbegin()->second;
        }

        T *previousItem(T *item) const {
            auto it = std::find_if(m_items.find(m_positions.value(item)), m_items.end(), [=](auto it) {
                return it.second == item;
            });
            if (it == m_items.end()) {
                return nullptr;
            }
            if (it == m_items.begin()) {
                return nullptr;
            }
            return (--it)->second;
        }

        T *nextItem(T *item) const {
            auto it = std::find_if(m_items.find(m_positions.value(item)), m_items.end(), [=](auto it) {
                return it.second == item;
            });
            if (it == m_items.end()) {
                return nullptr;
            }
            if (++it == m_items.end()) {
                return nullptr;
            }
            return it->second;
        }

        void insertItem(T *item, int position) {
            if (m_positions.contains(item)) {
                m_items.erase(std::find_if(m_items.find(m_positions.value(item)), m_items.end(), [=](auto it) {
                    return it.second == item;
                }));
            }
            m_items.insert({position, item});
            m_positions.insert(item, position);
        }

        void removeItem(T *item) {
            m_items.erase(std::find_if(m_items.find(m_positions.value(item)), m_items.end(), [=](auto it) {
                return it.second == item;
            }));
            m_positions.remove(item);
        }

        QList<T *> slice(int position, int length) const {
            if (position < 0) {
                return {};
            }
            QList<T *> ret;
            auto it = m_items.lower_bound(position + length);
            std::transform(m_items.lower_bound(position), it, std::back_inserter(ret), [](auto it) {
                return it.second;
            });
            return ret;
        }

        int size() const {
            return static_cast<int>(m_positions.size());
        }

        bool contains(T *item) const {
            return m_positions.contains(item);
        }
    };

}

#endif //DIFFSCOPE_DSPX_MODEL_POINTSEQUENCECONTAINER_P_H
