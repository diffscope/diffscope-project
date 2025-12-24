#ifndef DIFFSCOPE_DSPX_MODEL_RANGEHELPERS_H
#define DIFFSCOPE_DSPX_MODEL_RANGEHELPERS_H

#include <iterator>

namespace dspx::impl {

    template<class SequenceType>
    class SequenceRange {
    public:
        SequenceRange(const SequenceType *sequence) : m_sequence(sequence) {
        }

        using ItemType = std::remove_pointer_t<decltype(std::declval<SequenceType>().firstItem())>;

        class iterator {
        public:
            using iterator_category = std::bidirectional_iterator_tag;
            using value_type = ItemType *;
            using difference_type = std::ptrdiff_t;
            using pointer = ItemType **;
            using reference = ItemType *&;

            iterator(const SequenceType *sequence, ItemType *item) : m_sequence(sequence), m_item(item) {
            }

            value_type operator*() const {
                return m_item;
            }

            iterator &operator++() {
                if (m_item) {
                    m_item = m_sequence->nextItem(m_item);
                }
                return *this;
            }

            iterator operator++(int) {
                iterator temp = *this;
                ++(*this);
                return temp;
            }

            iterator &operator--() {
                if (m_item) {
                    m_item = m_sequence->previousItem(m_item);
                } else {
                    m_item = m_sequence->lastItem();
                }
                return *this;
            }

            iterator operator--(int) {
                iterator temp = *this;
                --(*this);
                return temp;
            }

            bool operator==(const iterator &other) const {
                return m_item == other.m_item;
            }

            bool operator!=(const iterator &other) const {
                return m_item != other.m_item;
            }

        private:
            const SequenceType *m_sequence;
            ItemType *m_item;
        };

        using const_iterator = iterator;
        using reverse_iterator = std::reverse_iterator<iterator>;
        using const_reverse_iterator = std::reverse_iterator<const_iterator>;

        iterator begin() const {
            return iterator(m_sequence, m_sequence->firstItem());
        }

        iterator end() const {
            return iterator(m_sequence, nullptr);
        }

        const_iterator cbegin() const {
            return begin();
        }

        const_iterator cend() const {
            return end();
        }

        reverse_iterator rbegin() const {
            return reverse_iterator(end());
        }

        reverse_iterator rend() const {
            return reverse_iterator(begin());
        }

        const_reverse_iterator crbegin() const {
            return rbegin();
        }

        const_reverse_iterator crend() const {
            return rend();
        }

    private:
        const SequenceType *m_sequence;
    };

}

#endif //DIFFSCOPE_DSPX_MODEL_RANGEHELPERS_H
